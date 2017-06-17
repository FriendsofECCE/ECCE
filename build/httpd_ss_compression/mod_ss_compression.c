/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 *
 * Portions of this software are based upon public domain software
 * (zlib functions gz_open and gzwrite)
 */

/*
 * mod_deflate.c: Perform deflate transfer-encoding on the fly
 *
 * Written by Ian Holsman (IanH@apache.org)
 *
 */

#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "apr_strings.h"
#include "apr_general.h"
#include "util_filter.h"
#include "apr_buckets.h"
#include "http_request.h"
#define APR_WANT_STRFUNC
#include "apr_want.h"

#include "zlib.h"

/* As part of the encoding process, we must send what our OS_CODE is
 * (or so it seems based on what I can tell of how gzip encoding works).
 *
 * zutil.h is not always included with zlib distributions (it is a private
 * header), so this is straight from zlib 1.1.3's zutil.h.
 */
#ifdef OS2
#define OS_CODE  0x06
#endif

#ifdef WIN32 /* Window 95 & Windows NT */
#define OS_CODE  0x0b
#endif

#if defined(VAXC) || defined(VMS)
#define OS_CODE  0x02
#endif

#ifdef AMIGA
#define OS_CODE  0x01
#endif

#if defined(ATARI) || defined(atarist)
#define OS_CODE  0x05
#endif

#if defined(MACOS) || defined(TARGET_OS_MAC)
#define OS_CODE  0x07
#endif

#ifdef __50SERIES /* Prime/PRIMOS */
#define OS_CODE  0x0F
#endif

#ifdef TOPS20
#define OS_CODE  0x0a
#endif

#ifndef OS_CODE
#define OS_CODE  0x03  /* assume Unix */
#endif


static const char deflateFilterName[] = "SS_COMPRESSION";
module AP_MODULE_DECLARE_DATA ss_compression_module;

typedef struct deflate_filter_config_t
{
    int windowSize;
    int memlevel;
    int compressionlevel;
    apr_size_t bufferSize;
    char *filter_type;
    char *note_ratio_name;
    char *note_input_name;
    char *note_output_name;
} deflate_filter_config;

/* windowsize is negative to suppress Zlib header */
#define DEFAULT_COMPRESSION Z_DEFAULT_COMPRESSION
#define DEFAULT_WINDOWSIZE -15
#define DEFAULT_MEMLEVEL 9
#define DEFAULT_BUFFERSIZE 8096
#define TRJ_FILTERTYPE "trj"
#define ALL_FILTERTYPE "all"
#define DEFAULT_FILTERTYPE TRJ_FILTERTYPE

/* Outputs a long in LSB order to the given file
 * only the bottom 4 bits are required for the deflate file format.
 */
static void putLong(unsigned char *string, unsigned long x)
{
    string[0] = (unsigned char)(x & 0xff);
    string[1] = (unsigned char)((x & 0xff00) >> 8);
    string[2] = (unsigned char)((x & 0xff0000) >> 16);
    string[3] = (unsigned char)((x & 0xff000000) >> 24);
}

/* Inputs a string and returns a long.
 */
static unsigned long getLong(unsigned char *string)
{
    return ((unsigned long)string[0])
          | (((unsigned long)string[1]) << 8)
          | (((unsigned long)string[2]) << 16)
          | (((unsigned long)string[3]) << 24);
}

static void *create_deflate_server_config(apr_pool_t *p, server_rec *s)
{
    deflate_filter_config *c = apr_pcalloc(p, sizeof *c);

    c->memlevel   = DEFAULT_MEMLEVEL;
    c->windowSize = DEFAULT_WINDOWSIZE;
    c->bufferSize = DEFAULT_BUFFERSIZE;
    c->compressionlevel = DEFAULT_COMPRESSION;
    c->filter_type = apr_pstrdup(p, DEFAULT_FILTERTYPE);

    return c;
}

static const char *deflate_set_window_size(cmd_parms *cmd, void *dummy,
                                           const char *arg)
{
    deflate_filter_config *c = ap_get_module_config(cmd->server->module_config,
                                                    &ss_compression_module);
    int i;

    i = atoi(arg);

    if (i < 1 || i > 15)
        return "sscWindowSize must be between 1 and 15";

    c->windowSize = i * -1;

    return NULL;
}

static const char *deflate_set_buffer_size(cmd_parms *cmd, void *dummy,
                                           const char *arg)
{
    deflate_filter_config *c = ap_get_module_config(cmd->server->module_config,
                                                    &ss_compression_module);
    int n = atoi(arg);

    if (n <= 0) {
        return "sscBufferSize should be positive";
    }

    c->bufferSize = (apr_size_t)n;

    return NULL;
}
static const char *deflate_set_note(cmd_parms *cmd, void *dummy,
                                    const char *arg1, const char *arg2)
{
    deflate_filter_config *c = ap_get_module_config(cmd->server->module_config,
                                                    &ss_compression_module);
    
    if (arg2 == NULL) {
        c->note_ratio_name = apr_pstrdup(cmd->pool, arg1);
    }
    else if (!strcasecmp(arg1, "ratio")) {
        c->note_ratio_name = apr_pstrdup(cmd->pool, arg2);
    }
    else if (!strcasecmp(arg1, "input")) {
        c->note_input_name = apr_pstrdup(cmd->pool, arg2);
    }
    else if (!strcasecmp(arg1, "output")) {
        c->note_output_name = apr_pstrdup(cmd->pool, arg2);
    }
    else {
        return apr_psprintf(cmd->pool, "Unknown note type %s", arg1);
    }

    return NULL;
}

static const char *deflate_set_memlevel(cmd_parms *cmd, void *dummy,
                                        const char *arg)
{
    deflate_filter_config *c = ap_get_module_config(cmd->server->module_config,
                                                    &ss_compression_module);
    int i;

    i = atoi(arg);

    if (i < 1 || i > 9)
        return "sscMemLevel must be between 1 and 9";

    c->memlevel = i;

    return NULL;
}

static const char *deflate_set_compressionlevel(cmd_parms *cmd, void *dummy,
                                        const char *arg)
{
    deflate_filter_config *c = ap_get_module_config(cmd->server->module_config,
                                                    &ss_compression_module);
    int i;

    i = atoi(arg);

    if (i < 1 || i > 9)
        return "Compression Level must be between 1 and 9";

    c->compressionlevel = i;

    return NULL;
}


static const char *deflate_set_filter_type(cmd_parms *cmd, void *dummy,
                                           const char *arg)
{
    deflate_filter_config *c = ap_get_module_config(cmd->server->module_config,
                                                    &ss_compression_module);
     
    if (arg == NULL) {
        c->filter_type = apr_pstrdup(cmd->pool, DEFAULT_FILTERTYPE);
    }
    else if (!strcasecmp(arg, "trj")) {
        c->filter_type = apr_pstrdup(cmd->pool, arg);
    }
    else if (!strcasecmp(arg, "all")) {
        c->filter_type = apr_pstrdup(cmd->pool, arg);
    }
    else {
        return apr_psprintf(cmd->pool, "Unknown filter type %s", arg);
    }

    return NULL;
}



/* magic header */
static char deflate_magic[2] = { '\037', '\213' };



typedef struct deflate_ctx_t
{
    z_stream stream;
    unsigned char *buffer;
    unsigned long crc;
    apr_bucket_brigade *bb, *proc_bb;
    int data_bucket_seen;
    int eos_bucket_seen;
    int deflate_hdr;
} deflate_ctx;


/*====================================================================
 *  Method: deflate_out_filter
 * Summary: Performs zlib decompression on the GET response body.
 *==================================================================*/
static apr_status_t deflate_out_filter(ap_filter_t *f,
                                       apr_bucket_brigade *bb)
{
    apr_bucket *e;
    request_rec *r = f->r;
    deflate_ctx *ctx = f->ctx;
    int zRC;
    deflate_filter_config *c;
    int remove_deflate_hdr = 0;

    apr_off_t bb_length;
    apr_status_t bb_length_status;
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                  "OUTPUT:: ************* ENTER OUTPUT FILTER ****************");
    bb_length_status = apr_brigade_length(bb,1,&bb_length);
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                  "OUTPUT:: BB LENGTH: %d", bb_length);

    c = ap_get_module_config(r->server->module_config, &ss_compression_module);


    /* Return here for a transparent filter */
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "OUTPUT:: INSIDE OUTPUT FILTER:: METHOD: %s", r->method);
    /* return ap_pass_brigade(f->next, bb); */


    /* Only inflate GET requests */
    if (strcasecmp(r->method, "GET") != 0) {
        return ap_pass_brigade(f->next, bb);
    }
    else {
      ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                    "OUTPUT:: FOUND A GET REQUEST WILL INFLATE IF NECESSARY");
    }


    /*----------------------Initialization----------------------------
     *
     * If there is no context then this is the initial call:
     *  - Only work on main requests (no sub-requests).
     *  - Make sure the deflate header is correct.
     *  - Initialize Zlib's internal decompression stream. (if everything
     *    looks ok).
     */
    if (!ctx) {

        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                      "OUTPUT:: NO CONTEXT, FIRST CALL FOR THIS REQUEST");

        char *buf, *token, deflate_hdr[10];
        const char *encoding, *accepts;
        apr_size_t len;
        apr_status_t rv;

        /* only work on main request/no subrequests */
        if (r->main) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                          "OUTPUT:: SUB-REQUEST PASS IT ON");
            ap_remove_output_filter(f);
            return ap_pass_brigade(f->next, bb);
        }

        /* Get deflate header - make sure it is a 
         * gzip resource 
         */
        len = 10; 
        rv = apr_brigade_flatten(bb, deflate_hdr, &len); 
        if (rv != APR_SUCCESS) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                          "OUTPUT:: ERROR WITH BRIGADE FLATTEN");
            ap_remove_output_filter(f);
            return ap_pass_brigade(f->next, bb);
        }

        /* We didn't get the magic bytes. */
        if (len != 10 ||
            deflate_hdr[0] != deflate_magic[0] ||
            deflate_hdr[1] != deflate_magic[1]) {

            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                          "OUTPUT:: MAGIC BYTES NOT FOUND");

            ap_remove_output_filter(f);
            return ap_pass_brigade(f->next, bb);
        }

        /* We can't handle flags for now. */
        if (deflate_hdr[3] != 0) {

            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                          "OUTPUT:: CAN NOT HANDLE FLAGS");

            ap_remove_output_filter(f);
            return ap_pass_brigade(f->next, bb);
        }

        /* So far so good - looks ok to deflate */
        ctx = f->ctx = apr_pcalloc(r->pool, sizeof(*ctx));
        ctx->bb = apr_brigade_create(r->pool, f->c->bucket_alloc);
        ctx->buffer = apr_palloc(r->pool, c->bufferSize);

        /* Ok to remove deflate header */
        remove_deflate_hdr = 1;

        /* Initialize internal decompression stream */
        zRC = inflateInit2(&ctx->stream, c->windowSize);
        if (zRC != Z_OK) {
            f->ctx = NULL;
            inflateEnd(&ctx->stream);
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                          "OUTPUT:: unable to init Zlib: "
                          "inflateInit2 returned %d: URL %s",
                          zRC, r->uri);

            ap_remove_output_filter(f);
            return ap_pass_brigade(f->next, bb);
        }

        /* Content-Length will change - so remove it */
        apr_table_unset(r->headers_out, "Content-Length");

        /* initialize deflate output buffer */
        ctx->stream.next_out = ctx->buffer;
        ctx->stream.avail_out = c->bufferSize;

        /* Clear brigade for processing */
        apr_brigade_cleanup(ctx->bb);
    }
    
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                  "OUTPUT:: ## BEGIN BUCKET LOOP ##");
    while (!APR_BRIGADE_EMPTY(bb)) 
    {
        const char *data;
        apr_bucket *b;
        apr_size_t len;
        int done = 0;


        /* Get the next bucket for processing */
        e = APR_BRIGADE_FIRST(bb);


        /*------ EOS BUCKET ------*/
        if (APR_BUCKET_IS_EOS(e)) {

            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                          "OUTPUT:: EOS BUCKET FOUND");

            /* Add to output brigade and pass on */
            APR_BUCKET_REMOVE(e);
            APR_BRIGADE_INSERT_TAIL(ctx->bb, e);
            ap_pass_brigade(f->next, ctx->bb);
            continue;
            
        }


        /*------ FLUSH BUCKET ------*/
        if (APR_BUCKET_IS_FLUSH(e)) {

            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                          "OUTPUT:: FLUSH BUCKET FOUND");

            apr_bucket *bkt;
            apr_status_t rv;
            apr_bucket_delete(e);

            if (ctx->stream.avail_in > 0) {
              zRC = inflate(&(ctx->stream), Z_SYNC_FLUSH);
              if (zRC != Z_OK) {
                  inflateEnd(&ctx->stream);
                  return APR_EGENERAL;
              }
            }


            ctx->stream.next_out = ctx->buffer;
            len = c->bufferSize - ctx->stream.avail_out;

            ctx->crc = crc32(ctx->crc, (const Bytef *)ctx->buffer, len);
            b = apr_bucket_heap_create((char *)ctx->buffer, len,
                                       NULL, f->c->bucket_alloc);
            APR_BRIGADE_INSERT_TAIL(ctx->bb, b);
            ctx->stream.avail_out = c->bufferSize;

            bkt = apr_bucket_flush_create(f->c->bucket_alloc);
            APR_BRIGADE_INSERT_TAIL(ctx->bb, bkt);
            rv = ap_pass_brigade(f->next, ctx->bb);
            if (rv != APR_SUCCESS) {
              return rv;
            }
            continue;
        }


        /*------ DATA BUCKET ------*/
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                      "OUTPUT:: DATA BUCKET FOUND");

        if (remove_deflate_hdr) {
          ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                        "OUTPUT:: REMOVING DEFLATE HEADER");
            apr_bucket_split(e, 10);
            apr_bucket_delete(e);
            remove_deflate_hdr = 0;
            continue;
        }

        /* Get Data Bucket contents */
        apr_bucket_read(e, &data, &len, APR_BLOCK_READ);
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                      "OUTPUT:: DATA BUCKET READ: READ %d BYTES", len);

        /* Pass through zlib inflate. */
        ctx->stream.next_in = (unsigned char *)data;
        ctx->stream.avail_in = len;

        zRC = Z_OK;

        while (ctx->stream.avail_in != 0) {
            if (ctx->stream.avail_out == 0) {
                apr_bucket *tmp_heap;
                apr_status_t rv;
                ctx->stream.next_out = ctx->buffer;
                len = c->bufferSize - ctx->stream.avail_out;
                ctx->crc = crc32(ctx->crc, (const Bytef *)ctx->buffer, len);
                b = apr_bucket_heap_create((char *)ctx->buffer, len,
                                                  NULL, f->c->bucket_alloc);
                APR_BRIGADE_INSERT_TAIL(ctx->bb, b);
                ctx->stream.avail_out = c->bufferSize;
                /* Send what we have right now to the next filter. */
                rv = ap_pass_brigade(f->next, ctx->bb);
                if (rv != APR_SUCCESS) {
                    return rv;
                }
            }

            zRC = inflate(&ctx->stream, Z_NO_FLUSH);

            if (zRC == Z_STREAM_END) {
                break;
            }

            if (zRC != Z_OK) {
                inflateEnd(&ctx->stream);
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                              "OUTPUT:: INFLATE ERROR: %d", zRC);
                return APR_EGENERAL;
            }
        }
        if (zRC == Z_STREAM_END) {

            apr_bucket *tmp_heap, *eos;
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                          "OUTPUT:: Zlib: Inflated %ld to %ld : URL %s",
                          ctx->stream.total_in, ctx->stream.total_out,
                          r->uri);

            len = c->bufferSize - ctx->stream.avail_out;
            ctx->crc = crc32(ctx->crc, (const Bytef *)ctx->buffer, len);
            tmp_heap = apr_bucket_heap_create((char *)ctx->buffer, len,
                                              NULL, f->c->bucket_alloc);
            APR_BRIGADE_INSERT_TAIL(ctx->bb, tmp_heap);
            ctx->stream.avail_out = c->bufferSize;

            /* Are the remaining 8 bytes already in the avail stream? */
            if (ctx->stream.avail_in >= 8) {
                unsigned long compCRC, compLen;
                compCRC = getLong(ctx->stream.next_in);
                if (ctx->crc != compCRC) {
                    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                                  "OUTPUT:: INCORRECT CRC");
                    inflateEnd(&ctx->stream);
                    return APR_EGENERAL;
                }
                ctx->stream.next_in += 4;
                compLen = getLong(ctx->stream.next_in);
                if (ctx->stream.total_out != compLen) {
                    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                                  "OUTPUT:: INCORRECT LENGTH");
                    inflateEnd(&ctx->stream);
                    return APR_EGENERAL;
                }
            }
            else {
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                              "OUTPUT:: COULD NOT FIND CRC "
                              "AND LENGTH (LAST 8 BYTES)");

                /* FIXME: We need to grab the 8 verification bytes
                 * from the wire! */
                inflateEnd(&ctx->stream);
                return APR_EGENERAL;
            }

            inflateEnd(&ctx->stream);

            /* Inflate should be complete - pass on brigade */
            ap_pass_brigade(f->next, ctx->bb);
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                          "OUTPUT:: INFLATE COMPLETE - PASSING BRIGADE");
        }
        apr_bucket_delete(e);
    }
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                  "OUTPUT:: ## END BUCKET LOOP ##");

    apr_brigade_cleanup(bb);

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                  "OUTPUT:: ************* LEAVE OUTPUT FILTER ****************");

    return APR_SUCCESS;
}



/*====================================================================
 *  Method: deflate_in_filter
 * Summary: Performs zlib compression on PUT request body.
 *==================================================================*/
static apr_status_t deflate_in_filter(ap_filter_t *f,
                                      apr_bucket_brigade *bb,
                                      ap_input_mode_t mode,
                                      apr_read_type_e block,
                                      apr_off_t readbytes)
{
    apr_bucket *bkt;
    request_rec *r = f->r;
    deflate_ctx *ctx = f->ctx;
    int zRC;
    apr_status_t rv;
    deflate_filter_config *c;
    const char *encoding;
    apr_bucket *e;
    char *buf;


    /* Return here for a transparent filter */
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                  "INPUT:: INSIDE INPUT FILTER:: METHOD: %s ::"
                  " READBYTES: %d :: MODE: %d :: BLOCK %d",
                  r->method, readbytes, mode, block);
    /* return ap_get_brigade(f->next, bb, mode, block, readbytes); */



    /* AP_MODE_READBYTES only acceptable mode */
    if (mode != AP_MODE_READBYTES) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                      "INPUT:: MODE IS NOT READBYTES:: MODE: %d", mode);
        return ap_get_brigade(f->next, bb, mode, block, readbytes);
    }
    /* Only compress PUT request bodies */
    if (strcasecmp(r->method, "PUT") != 0) {
      return ap_get_brigade(f->next, bb, mode, block, readbytes);
    }
    /* Must have a content type */
    if (r->content_type == NULL) {
      return ap_get_brigade(f->next, bb, mode, block, readbytes);
    }


    /* Get filter confiuration data */
    c = ap_get_module_config(r->server->module_config, &ss_compression_module);

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                  "INPUT:: FILTER TYPE :: %s", c->filter_type);


    /* TRJ : Only compress if it is a trajectory content type */
    if (strcasecmp(TRJ_FILTERTYPE, c->filter_type) == 0) {
      if (strcasecmp("chemical/x-nwchem-md-trajectory", r->content_type)) {
        return ap_get_brigade(f->next, bb, mode, block, readbytes);
      }
      else {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                      "INPUT:: FOUND TRAJECTORY PUT REQUEST TO COMPRESS");
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                      r->content_type);
      }
    }
    /* ALL : Only compress if it is one of the following content types */
    else if (strcasecmp(ALL_FILTERTYPE, c->filter_type) == 0) {
      if (strcasecmp("chemical/x-ecce-mvm", r->content_type) &&
          strcasecmp("chemical/x-gaussian-cube", r->content_type) &&
          strcasecmp("chemical/x-pdb", r->content_type) &&
          strcasecmp("chemical/x-nwchem-output", r->content_type) &&
          strcasecmp("chemical/x-nwchem-mo", r->content_type) &&
          strcasecmp("chemical/x-nwchem-restart", r->content_type) &&
          strcasecmp("chemical/x-nwchem-topology", r->content_type) &&
          strcasecmp("chemical/x-nwchem-md-properties", r->content_type) &&
          strcasecmp("chemical/x-nwchem-md-output", r->content_type) &&
          strcasecmp("chemical/x-nwchem-md-trajectory", r->content_type) &&
          strcasecmp("chemical/x-gaussian03-output", r->content_type) &&
          strcasecmp("chemical/x-gaussian98-output", r->content_type) &&
          strcasecmp("chemical/x-gaussian94-output", r->content_type) &&
          strcasecmp("chemical/x-gaussian92-output", r->content_type) ) {
        return ap_get_brigade(f->next, bb, mode, block, readbytes);
      }
      else {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                      "INPUT:: FOUND ALL PUT REQUEST TO COMPRESS");
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                      r->content_type);
      }
    }
    /* Invalid filter type, go ahead and return */
    else {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                      "INPUT:: INVALID FILTER TYPE");
        return ap_get_brigade(f->next, bb, mode, block, readbytes);
    }



    /*----------------------Initialization----------------------------
     *
     * If there is no context then this is the initial call:
     *  - Only work on main requests (no sub-requests).
     *  - Make sure Content-Encoding is not gzip (already compressed).
     *  - Set a flag indicating the the deflate header should be added
     *    to the input stream.
     *  - Initialize Zlib's internal compression stream. (if everything
     *    looks ok).
     */
    if (!ctx) {

        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                      "INPUT:: INITIAL CALL - NO CONTEXT");

        int found = 0;
        char *token, deflate_hdr[10];
        apr_size_t len;

        /* only work on main request/no subrequests */
        if (r->main) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                          "INPUT:: FOUND A SUBREQUEST");
            ap_remove_input_filter(f);
            return ap_get_brigade(f->next, bb, mode, block, readbytes);
        }

        /* Check Content-Encoding, if gzip is present, don't gzip again. */
        encoding = apr_table_get(r->headers_in, "Content-Encoding");
        if (encoding) {
            const char *tmp = encoding;

            token = ap_get_token(r->pool, &tmp, 0);
            while (token && token[0]) {
                if (!strcasecmp(token, "gzip")) {
                    found = 1;
                    break;
                }
                /* Otherwise, skip token */
                tmp++;
                token = ap_get_token(r->pool, &tmp, 0);
            }
        }

        /* gzip is present */
        if (found == 1) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                          "INPUT:: GZIP ENCODED ALREADY");
            ap_remove_input_filter(f);
            return ap_get_brigade(f->next, bb, mode, block, readbytes);
        }


        /* So far so good - looks ok to deflate */
        f->ctx = ctx = apr_pcalloc(f->r->pool, sizeof(*ctx));
        ctx->bb = apr_brigade_create(r->pool, f->c->bucket_alloc);
        ctx->proc_bb = apr_brigade_create(r->pool, f->c->bucket_alloc);
        ctx->buffer = apr_palloc(r->pool, c->bufferSize);
        ctx->eos_bucket_seen = 0;
        ctx->data_bucket_seen = 0;
        ctx->deflate_hdr = 0;


        /* Initialize internal zlib compression stream */
        zRC = deflateInit2(&ctx->stream, c->compressionlevel, Z_DEFLATED,
                           c->windowSize, c->memlevel,
                          Z_DEFAULT_STRATEGY);


        if (zRC != Z_OK) {
            f->ctx = NULL;
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
                          "INPUT:: unable to init Zlib: deflateInit2"
                          " returned %d: URL %s", zRC, r->uri);
            ap_remove_input_filter(f);
            return ap_get_brigade(f->next, bb, mode, block, readbytes);
        }

        /* initialize deflate output buffer */
        ctx->stream.next_out = ctx->buffer;
        ctx->stream.avail_out = c->bufferSize;
    }



    /*--------------------------Processing--------------------------*/
    if (APR_BRIGADE_EMPTY(ctx->proc_bb)) {
 
        /* Request upstream input to process */
        rv = ap_get_brigade(f->next, ctx->bb, mode, block, readbytes);

        if (rv != APR_SUCCESS) {
            return rv;
        }


        /* Process bucket brigade */
        for (bkt = APR_BRIGADE_FIRST(ctx->bb);
             bkt != APR_BRIGADE_SENTINEL(ctx->bb);
             bkt = APR_BUCKET_NEXT(bkt)) {



            const char *data;
            apr_bucket *b, *eos;
            apr_size_t len;
            int done = 0;



            /*------ EOS BUCKET ------*/
            if (APR_BUCKET_IS_EOS(bkt)) {

                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                              "INPUT:: FOUND - EOS BUCKET");

                char *buf;
                unsigned int deflate_len;

                if (!ctx->eos_bucket_seen) { 

                    /* We've seen the EOS bucket */
                    ctx->eos_bucket_seen = 1;

                    /* Make sure there is data to compress, otherwise it may
                     * be an empty file 
                     */
                    if (ctx->data_bucket_seen) {

                        /* Should already be zero */
                        ctx->stream.avail_in = 0;


                        /* Flush remaining zlib output to brigade */
                        for (;;) {
    
                            deflate_len = c->bufferSize - ctx->stream.avail_out;
                            if (deflate_len != 0) {
                                b = apr_bucket_heap_create((char *)ctx->buffer,
                                                           deflate_len, NULL,
                                                           f->c->bucket_alloc);
                                APR_BRIGADE_INSERT_TAIL(ctx->proc_bb, b);
                                ctx->stream.next_out = ctx->buffer;
                                ctx->stream.avail_out = c->bufferSize;
                            }
        
                            if (done) {
                                break;
                            }
        
                            zRC = deflate(&ctx->stream, Z_FINISH);
        
                            if (deflate_len == 0 && zRC == Z_BUF_ERROR) {
                                zRC = Z_OK;
                            }
        
                            done = (ctx->stream.avail_out != 0 || zRC == Z_STREAM_END);
        
                            if (zRC != Z_OK && zRC != Z_STREAM_END) {
                                break;
                            }
                        }


                        /* Add CRC and Bytes compressed to stream */
                        buf = apr_palloc(r->pool, 8);
                        putLong((unsigned char *)&buf[0], ctx->crc);
                        putLong((unsigned char *)&buf[4], ctx->stream.total_in);
                        b = apr_bucket_pool_create(buf, 8, r->pool, f->c->bucket_alloc);
                        APR_BRIGADE_INSERT_TAIL(ctx->proc_bb, b);

                        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                                      "INPUT:: Zlib: Compressed %ld to %ld : URL %s",
                                     ctx->stream.total_in, ctx->stream.total_out, r->uri);
        
                        /* Leave notes for logging */
                        if (c->note_input_name) {
                            apr_table_setn(r->notes, c->note_input_name,
                                           (ctx->stream.total_in > 0)
                                            ? apr_off_t_toa(r->pool,
                                                            ctx->stream.total_in)
                                            : "-");
                        }
                        if (c->note_output_name) {
                            apr_table_setn(r->notes, c->note_output_name,
                                           (ctx->stream.total_in > 0)
                                            ? apr_off_t_toa(r->pool,
                                                            ctx->stream.total_out)
                                            : "-");
                        }
                        if (c->note_ratio_name) {
                            apr_table_setn(r->notes, c->note_ratio_name,
                                          (ctx->stream.total_in > 0)
                                           ? apr_itoa(r->pool,
                                                       (int)(ctx->stream.total_out
                                                             * 100
                                                            / ctx->stream.total_in))
                                           : "-");
                        }

                    }

                    /* EOS bucket has been processed - free compression stream */
                    deflateEnd(&ctx->stream);
                }

                /* Remember to add EOS bucket to processing bucket brigade */
                APR_BUCKET_REMOVE(bkt);
                APR_BRIGADE_INSERT_TAIL(ctx->proc_bb, bkt);
                break;
            }


            /*------ FLUSH BUCKET ------*/
            if (APR_BUCKET_IS_FLUSH(bkt)) {

                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                              "INPUT:: FOUND - FLUSH BUCKET");

                apr_bucket *tmp_heap;

                /* Make sure to save any processing that has been 
                 * completed up to this point 
                 */
                zRC = deflate(&(ctx->stream), Z_SYNC_FLUSH);
                if (zRC != Z_OK) {
                    return APR_EGENERAL;
                }
                ctx->stream.next_out = ctx->buffer;
                len = c->bufferSize - ctx->stream.avail_out;

                tmp_heap = apr_bucket_heap_create((char *)ctx->buffer, len,
                                                 NULL, f->c->bucket_alloc);
                APR_BRIGADE_INSERT_TAIL(ctx->proc_bb, tmp_heap);
                ctx->stream.avail_out = c->bufferSize;

                /* Pass on what we have */
                APR_BUCKET_REMOVE(bkt);
                APR_BRIGADE_CONCAT(bb, ctx->bb);
                break;
            }


            /*------ DATA BUCKET ------*/
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                          "INPUT:: FOUND - DATA BUCKET");

            /* Get bucket contents */
            ctx->data_bucket_seen = 1;
            apr_bucket_read(bkt, &data, &len, APR_BLOCK_READ);
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                          "INPUT:: READ BUCKET DATA:: READ: %d", len);

    
            /* Update a running CRC for input stream */
            ctx->crc = crc32(ctx->crc, (const Bytef *)data, len);


            /* Pass through zlib deflate. */
            ctx->stream.next_in = (unsigned char *)data;
            ctx->stream.avail_in = len;
            
            while (ctx->stream.avail_in != 0) {
                if (ctx->stream.avail_out == 0) {
                    ctx->stream.next_out = ctx->buffer;
                    len = c->bufferSize - ctx->stream.avail_out;

                    b = apr_bucket_heap_create((char *)ctx->buffer, len,
                                               NULL, f->c->bucket_alloc);
                    APR_BRIGADE_INSERT_TAIL(ctx->proc_bb, b);
                    ctx->stream.avail_out = c->bufferSize;
                }

                zRC = deflate(&(ctx->stream), Z_NO_FLUSH);

                if (zRC != Z_OK) {
                    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                                  "INPUT:: DEFLATE ERROR ON DATA BUCKET");
                    return APR_EGENERAL;
                }
            }

        }

        apr_brigade_cleanup(ctx->bb);
    }


    /*--------------------------Cleanup-------------------------------
     * Make sure to return contents of zlib buffer along with 
     * contents of the processing bucket brigade (ctx->proc_bb).
     * At most readbytes should be returned in the brigade.
     * If deflate header has not been added, go ahead and add it.
     */


    /* If the EOS bucket has not been seen yet, flush zlib stream */
    if (!ctx->eos_bucket_seen) {
            apr_bucket *tmp1_heap;
            apr_size_t lenbuf;

            zRC = deflate(&(ctx->stream), Z_SYNC_FLUSH);

            if (zRC != Z_OK) {
                return APR_EGENERAL;
            }
            ctx->stream.next_out = ctx->buffer;
            lenbuf = c->bufferSize - ctx->stream.avail_out;

            tmp1_heap = apr_bucket_heap_create((char *)ctx->buffer, lenbuf,
                                             NULL, f->c->bucket_alloc);
            APR_BRIGADE_INSERT_TAIL(ctx->proc_bb, tmp1_heap);
            ctx->stream.avail_out = c->bufferSize;
    }


    /* Add deflate header if there is data to compress and
     * the EOS bucket has not been processed yet 
     */
    if (!ctx->deflate_hdr && ctx->data_bucket_seen) {

        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                      "INPUT:: ADDING DEFLATE HEADER");

        /* RFC 1952 Section 2.3 dictates the gzip header:
         *
         * +---+---+---+---+---+---+---+---+---+---+
         * |ID1|ID2|CM |FLG|     MTIME     |XFL|OS |
         * +---+---+---+---+---+---+---+---+---+---+
         *
         * If we wish to populate in MTIME (as hinted in RFC 1952), do:
         * putLong(date_array, apr_time_now() / APR_USEC_PER_SEC);
         * where date_array is a char[4] and then print date_array in the
         * MTIME position.  WARNING: ENDIANNESS ISSUE HERE.
         */
        buf = apr_psprintf(r->pool, "%c%c%c%c%c%c%c%c%c%c", deflate_magic[0],
                           deflate_magic[1], Z_DEFLATED, 0 /* flags */,
                           0, 0, 0, 0 /* 4 chars for mtime */,
                           0 /* xflags */, OS_CODE);
        e = apr_bucket_pool_create(buf, 10, r->pool, f->c->bucket_alloc);
        APR_BRIGADE_INSERT_HEAD(ctx->proc_bb, e);
        ctx->deflate_hdr = 1;
    }


    /* Pass on processed input to calling filter/generator */
    if (!APR_BRIGADE_EMPTY(ctx->proc_bb)) {

        apr_bucket_brigade *newbb;

        /* May return APR_INCOMPLETE which is fine by us. */
        apr_brigade_partition(ctx->proc_bb, readbytes, &bkt);
        newbb = apr_brigade_split(ctx->proc_bb, bkt);
        APR_BRIGADE_CONCAT(bb, ctx->proc_bb);
        APR_BRIGADE_CONCAT(ctx->proc_bb, newbb);
    }

    return APR_SUCCESS;
}



static void register_hooks(apr_pool_t *p)
{
    ap_register_output_filter(deflateFilterName, deflate_out_filter, NULL,
                              AP_FTYPE_CONTENT_SET);
    ap_register_input_filter(deflateFilterName, deflate_in_filter, NULL,
                              AP_FTYPE_CONTENT_SET);
}



static const command_rec deflate_filter_cmds[] = {
    AP_INIT_TAKE12("sscFilterNote", deflate_set_note, NULL, RSRC_CONF,
                  "Set a note to report on compression ratio"),
    AP_INIT_TAKE1("sscWindowSize", deflate_set_window_size, NULL,
                  RSRC_CONF, "Set the Deflate window size (1-15)"),
    AP_INIT_TAKE1("sscBufferSize", deflate_set_buffer_size, NULL, RSRC_CONF,
                  "Set the Deflate Buffer Size"),
    AP_INIT_TAKE1("sscMemLevel", deflate_set_memlevel, NULL, RSRC_CONF,
                  "Set the Deflate Memory Level (1-9)"),
    AP_INIT_TAKE1("sscCompressionLevel", deflate_set_compressionlevel, NULL, RSRC_CONF,
                  "Set the Deflate Compression Level (1-9)"),
    AP_INIT_TAKE1("sscFilterType", deflate_set_filter_type, NULL, RSRC_CONF,
                  "Set the type of files to be filtered, either only trajectory files or all files (\"trj\",\"all\")"),
    {NULL}
};



module AP_MODULE_DECLARE_DATA ss_compression_module = {
    STANDARD20_MODULE_STUFF,
    NULL,                         /* dir config creater */
    NULL,                         /* dir merger --- default is to override */
    create_deflate_server_config, /* server config */
    NULL,                         /* merge server config */
    deflate_filter_cmds,          /* command table */
    register_hooks                /* register hooks */
};


