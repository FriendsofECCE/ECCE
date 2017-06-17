/*
 * ConnollyDotFxns.c++
 *
 *     Functions which are called to generate the Connolly dot surface.
 *     This also contains the ChemConnollyDot::surface method.
 *
 *     
 * Copyright 1996, 1997, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * 
 * UNPUBLISHED -- Rights reserved under the copyright laws of the United
 * States.   Use of a copyright notice is precautionary only and does not
 * imply publication or disclosure.
 * 
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in FAR 52.227.19(c)(2) or subparagraph (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS 252.227-7013 and/or
 * in similar or successor clauses in the FAR, or the DOD or NASA FAR
 * Supplement.  Contractor/manufacturer is Silicon Graphics, Inc.,
 * 2011 N. Shoreline Blvd. Mountain View, CA 94039-7311.
 * 
 * THE CONTENT OF THIS WORK CONTAINS CONFIDENTIAL AND PROPRIETARY
 * INFORMATION OF SILICON GRAPHICS, INC. ANY DUPLICATION, MODIFICATION,
 * DISTRIBUTION, OR DISCLOSURE IN ANY FORM, IN WHOLE, OR IN PART, IS STRICTLY
 * PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF SILICON  
 * GRAPHICS, INC.
 */
 /**************************************************************************\
 *
 * OpenMOIV - C++ library for molecular visualization using Inventor.
 * Copyright (C) 2001-2003 Universitat Pompeu Fabra - Barcelona (Spain)
 * 
 * Developers: Interactive Technology Group (GTI)
 * Team: Josep Blat, Eduard Gonzalez, Sergi Gonzalez,
 *       Daniel Soto, Alejandro Ramirez, Oscar Civit.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details (see the file 
 * LICENSE.LGPL at the root directory).
 *
 * REMARK: This library is a derived product.
 *         You need also to accept all other applicable licenses.
 * 
 * Homepage: http://www.tecn.upf.es/openMOIV/
 * Contact:  openmoiv@upf.es
 *
\**************************************************************************/

#ident "$Revision: 22364 $"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __coin
#include "inv/system/inttypes.H"
#else
#include <inttypes.h>
#endif

#include "inv/ChemKit/SparseBits.H"

#include "inv/ChemKit/ChemConnollyDot.H"

/* MPM 12/2000 required by gcc */
#define SP_ALLOC_INC 1024

/*
 * The entry into these functions is through ChemConnollyDot::surface
 */

/* Static functions */
static void
CONNOLLYcircle_circle_intersection(float a[3], float b[3], float ra,
    float rb, float perpendicular[3], float base[3], float altitude[3],
    int32_t *intersect);

static int
CONNOLLYcollide(int32_t jnbr, int32_t knbr, int32_t num_neighbors,
    int32_t *neighbor_list, RWBitVec *mutual_neighbor, float *atom_radii,
    float atom_coords[][3], int32_t shape, float probe_radius, float cw[3]);

static int
CONNOLLYdo_concave(int32_t jnbr, int32_t knbr,
    int32_t iatom, int32_t jatom, int32_t katom,
    int32_t num_neighbor, int32_t *neighbor_list, RWBitVec *mutual_neighbor, 
    int8_t *surface_request, float *atom_radii, float atom_coords[][3], 
    float probe_radius, float height, float base[3], float uij2[3], 
    int32_t pair[2], float cw[2][3], float alt2[2][3]);

static int
CONNOLLYdo_reentrant(int32_t iatom, int32_t natoms, int8_t *surface_request, 
    float *atom_radii, float atom_coords[][3], int32_t *atom_index, 
    float probe_radius, int32_t num_points_on_probe, float density_of_points, 
    float probe_points[][3], int32_t num_circumference, float circle[][3], 
    int32_t num_neighbor, int32_t *neighbor_list,
    RWBitVec *mutual_neighbor_list, 
    float *d_matrix, RWBitVec *ayon, float *arcs, int32_t *total_probe_pos, 
    int32_t *total_surf_points, int32_t *total_yon_probes,
    int32_t *current_sp_allocated, 
    probe_descriptor *probe_pos, sp_descriptor **surf_points,
    int32_t *yon_probes);

static int
CONNOLLYgen_concave(float probe_radius, int32_t num_points_on_probe, 
    float probe_points[][3], int32_t pair[2], float cw[2][3], 
    float alt2[2][3], int32_t index_i, int32_t surface_i, float radius_i, 
    float coord_i[3], int32_t index_j, int32_t surface_j, float radius_j, 
    float coord_j[3], int32_t index_k, int32_t surface_k, float radius_k, 
    float coord_k[3], int32_t *total_probe_pos, int32_t *total_surf_points, 
    int32_t *total_yon_probes, int32_t *current_sp_allocated,
    probe_descriptor *probe_pos, sp_descriptor **surf_points,
    int32_t *yon_probes);

static int
CONNOLLYgen_contact(int32_t iatom, float *atom_radii, 
    float atom_coords[][3], int32_t *atom_index, float probe_radius, 
    float density_of_points, float contact_points[][3], int32_t num_neighbor, 
    int32_t *neighbor_list, int32_t *total_surf_points, int32_t *current_sp_allocated, 
    sp_descriptor **surf_points);

static int
CONNOLLYgen_saddle(int32_t jnbr, int32_t iatom, int32_t jatom,
    int32_t num_neighbor, 
    int32_t *neighbor_list, int8_t *surface_request, float *atom_radii, 
    float atom_coords[][3], int32_t *atom_index, float probe_radius, 
    float density_of_points, int32_t num_circumference, float circle[][3], 
    float height, float alt[3], float base[3], float uij2[3], 
    RWBitVec *ayon, float *arca, float arc0[][3], float arc1[][3], 
    int32_t *total_probe_pos, int32_t *total_surf_points,
    int32_t *total_yon_probes, 
    int32_t *current_sp_allocated, probe_descriptor *probe_pos,
    sp_descriptor **surf_points, int32_t *yon_probes);

static void
CONNOLLYremove_overlaps(float probe_radius, int32_t total_probe_pos, 
    int32_t total_yon_probes, probe_descriptor *probe_pos, 
    sp_descriptor *surf_points, int32_t *yon_probes, int32_t *victim_probes, 
    int32_t *eater_probes);

static void
CONNOLLYwrite_all(float probe_radius, int32_t total_probe_pos, 
    probe_descriptor *probe_pos, sp_descriptor *surf_points);

static void
CONNOLLYsort_surf_points(int32_t *total_surf_points, 
    sp_descriptor *surf_points);

static int
CONNOLLYcompar(const void *sp1, const void *sp2);

static int
CONNOLLYint_compar(int32_t int1, int32_t int2);

static int
CONNOLLYfloat_compar(float float1, float float2);

static void
CONNOLLYgen_unit_vectors(float probe_points[][3],
    int32_t *num_points_on_probe);

static int
CONNOLLYoffset(int32_t n, int32_t i, int32_t j);

static float
CONNOLLYdist(float a[3], float b[3]);

static float
CONNOLLYdist2(float a[3], float b[3]);

static float
CONNOLLYanorm(float a[3]);

static float
CONNOLLYdot(float a[3], float b[3]);

static void
CONNOLLYcross(float a[3], float b[3], float c[3]);

static void
CONNOLLYmultv(float v[3], float a[3][3], float w[3]);

static void
CONNOLLYvnorm(float a[3], float b[3]);

static void
CONNOLLYvperp(float a[3], float b[3]);

static float
CONNOLLYdet(float a[3], float b[3], float c[3]);

static void
CONNOLLYconcat(float a[3][3], float b[3][3]);

static void
CONNOLLYconjugate(float h[3][3], float g[3][3], float ghgt[3][3]);

static void
CONNOLLYidentity_matrix(float a[3][3]);

static void
error_return(const char *format, const char *args);

/* End of static functions */
// --> compatibility issues
//#define abs(a) ((a) < 0.0 ? -(a) : (a))
//#define min(a,b) ((a) < (b) ? (a) : (b))
//#define max(a,b) ((a) > (b) ? (a) : (b))
#ifndef abs
#define abs(a) ((a) < 0.0 ? -(a) : (a))
#endif
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
// <-- compatibility issues

int
ChemConnollyDot::surface(int32_t natoms, float probe_radius,
    float density_of_points,
    int8_t *surface_request, float *atom_radii, float *atom_coords,
    int32_t *atom_index, int32_t *total_returned,
    sp_descriptor **the_surface)
{
   int32_t i;
   int32_t coff;
   int32_t doff;
   int32_t current_sp_allocated;
   int32_t iatom;
   int32_t jatom;
   int32_t num_contacts;
   int32_t num_circumference;
   int32_t num_points_on_probe;
   int32_t num_neighbor;
   int32_t max_num_neighbor;
   int32_t num_concave_reent_probe_pos;
   int32_t num_saddle_reent_probe_pos;
   int32_t num_probe_pos;
   int32_t total_yon_probes;
   int32_t total_probe_pos;
   int32_t total_surf_points;
   int32_t *neighbor_list;
   RWBitVec *ayon;
   RWBitVec *mutual_neighbor_list;
   float d1;
   float fi;
   float min_radius;
   float max_radius;
   float sum_radius;
   float min_volume;
   float max_volume;

   int32_t *yon_probes;
   float *arcs;
   float *circle;
   float *contact_points;
   float *d_matrix;
   float *probe_points;

   probe_descriptor *probe_pos;
   sp_descriptor *surf_points;

   /*
    * If the probe_radius is 0 then we are just doing the Van der Waals
    * surface.  
    */

   /*
    * Generate a distance matrix.
    */

   if (!(d_matrix = new float[natoms*(natoms+1)/2])) {
      error_return ( "%s", "Could not allocate space for D_MATRIX." );
      return (0);
   }
   doff = 0;
   for (iatom = 0; iatom < natoms-1; iatom++) {
      for (jatom = iatom+1; jatom < natoms; jatom++) {
         d_matrix[doff++] = 
            CONNOLLYdist2 ( &atom_coords[iatom], &atom_coords[jatom] );
      }
   }

   /*
    * Allocate space for the neighbor_list and mutual_neighbor_list
    * based on the largest atom radius.
    *
    * 1) Determine the largest and smallest atom radius.
    * 2) The maximum distance that 2 atoms can be apart and considered
    *    neighbors is 2 * (max_radius + probe_radius).
    * 3) The volume of a sphere with that would contain atoms separated
    *    by this maximum distance would be:
    *       volume = 4/3 * M_PI * (radius**3)
    *    where radius = max_radius + probe_radius.
    * 4) Determine the volume occupied by the smallest radius.
    * 5) Divide the volume from (3) by the volume from (4) to figure out
    *    how many atoms will fit in the volume.
    */

   min_radius =  999.99f;
   max_radius = -999.99f;
   for (i = 0; i < natoms; i++) {
      min_radius = min ( min_radius, atom_radii[i] );
      max_radius = max ( max_radius, atom_radii[i] );
   }
   if (probe_radius < 1.4) {
      sum_radius = max_radius + 1.4;
   } else {
      sum_radius = max_radius + probe_radius;
   }
   max_volume = 4.0 * M_PI * sum_radius * sum_radius * sum_radius / 3.0;
   min_volume = 4.0 * M_PI * min_radius * min_radius * min_radius / 3.0;
   max_num_neighbor = (int)(max_volume/min_volume) + 1;

   /*
    * I haven't figured out the error in the equation for max_num_neighbor
    * above so temporarily I will use the following:
    */

   max_num_neighbor = natoms;

   if (!(neighbor_list = new int32_t[max_num_neighbor])) {
      error_return ( "%s", "Could not allocate space for NEIGHBOR_LIST." );
      return (0);
   }

   /*
    * Allocate space for the contact point32_t list based on the largest atom
    * radius.
    */
   // cast to int32_t added by fabien fontaine the 14/12/2000
   num_contacts = (int32_t) (4.0 * M_PI * max_radius * max_radius * density_of_points) ;

   if (!(contact_points = new float[3 * num_contacts])) {
      error_return ( "%s", "Could not allocate space for CONTACT_POINTS." );
      return (0);
   }
   
   /*
    * Allocate space for the surface points.  We will allocate
    * them as needed in chunks of SP_ALLOC_INC.
    */
   
   if (!(surf_points = 
      (sp_descriptor *) malloc ( SP_ALLOC_INC * sizeof(sp_descriptor) ))) {
      error_return ( "%s", "Could not allocate space for SURF_POINTS." );
      return (0);
   }
   current_sp_allocated = SP_ALLOC_INC;

   /*
    * At this point32_t we have allocated space for everything we need to generate
    * a "contact_only" surface.  Now allocate space for reentrant surface
    * arrays.
    */

   if (probe_radius != 0.0) {
      if (!(mutual_neighbor_list = new RWBitVec(max_num_neighbor, FALSE))) {
         error_return ( "%s", 
                    "Could not allocate space for MUTUAL_NEIGHBOR_LIST." );
         return (0);
      }
   
      /*
       * Set up the probe sphere and circle.
       * 4 * M_PI * R * R is the surface area of a sphere.
       */
      // cast to int32_t added by fabien fontaine the 14/12/2000
      num_points_on_probe = (int32_t) (4.0 * M_PI * probe_radius*probe_radius 
                              * density_of_points);
      if (num_points_on_probe < 1 ) num_points_on_probe = 1;
      if (!(probe_points = new float[3 * num_points_on_probe])) {
         error_return ( "%s", "Could not allocate space for PROBE_POINTS." );
         return (0);
      }
      CONNOLLYgen_unit_vectors ( (float (*)[3])probe_points,
         &num_points_on_probe );
      // cast to int32_t added by fabien fontaine the 14/12/2000
      num_circumference = (int32_t) (2.0 * M_PI * probe_radius * sqrt(density_of_points));
      if (num_circumference < 1) num_circumference = 1;
      if (!(circle = new float[3 * num_circumference])) {
         error_return ( "%s", "Could not allocate space for CIRCLE." );
         return (0);
      }

      coff = 0;
      for (i = 0; i < num_circumference; i++) {
         fi = (2.0 * M_PI * i) / num_circumference;
         circle[coff++] = probe_radius * cos(fi);
         circle[coff++] = probe_radius * sin(fi);
         circle[coff++] = 0.0;
      }
      /*
       * Allocate space for the probe positions based on the 
       * number of neighbors.
       * For concave reentrant surfaces the number of probe positions will be
       * bounded by num_neighbor * 2.  For saddle reentrant surfaces the
       * number of probe positions will be bounded by 
       * num_neighbor * 2 * num_rotations_about_iatom_jatom_axis.  This latter
       * quantity is determined by:
       *    num_rotations = sqrt (density_of_points) * M_PI * average
       * where average is the average of the following distances:
       *    the distance from the arc point32_t on iatom to the iatom-jatom axis
       *    the distance from the arc point32_t on jatom to the iatom-jatom axis
       *    the distance from the bottom point32_t of the arc to the iatom-jatom 
       *        axis
       * As an upper bound we can consider the probe radius and the iatom and
       * jatom radii to be max_radius.  We can also consider the 3 distances
       * listed above to be max_radius.  Thus we have:
       *    num_rotations = sqrt (density_of_points) * M_PI * max_radius.
       */
   
      num_concave_reent_probe_pos = max_num_neighbor * 2;
      // cast to int32_t added by fabien fontaine the 14/12/2000
      num_saddle_reent_probe_pos =  (int32_t) (max_num_neighbor * 2 
         * sqrt(density_of_points) * M_PI * max_radius) ;
   
      if (!(probe_pos =
         new probe_descriptor[num_concave_reent_probe_pos +
                              num_saddle_reent_probe_pos])) {
         error_return ("%s", "Could not allocate space for PROBE_POS." );
         return (0);
      }
      num_probe_pos = num_concave_reent_probe_pos + num_saddle_reent_probe_pos;
      if (!(yon_probes = new int32_t[3 * num_probe_pos])) {
         error_return ( "%s", "Could not allocate space for YON_PROBES." );
         return (0);
      }

      /*
       * Allocate space for some temporary arrays used in gen_saddle.
       * These are:
       *    ayon   -   dimensioned num_circumference max. (int)
       *    arca   -   dimensioned num_circumference max. (float)
       *    arc0   -   dimensioned num_circumference x 3 max. (float)
       *    arc1   -   dimensioned num_circumference x 3 max. (float)
       */
   
      if (!(ayon = new RWBitVec(num_circumference, FALSE))) {
         error_return ( "%s", "Could not allocate space for AYON." );
         return (0);
      }
      if (!(arcs = new float[7 * num_circumference])) {
         error_return ( "%s", "Could not allocate space for ARCS." );
         return (0);
      }

   } /* end if (probe_radius != 0.0) */

   /*
    * Loop through the atoms.
    */

   total_yon_probes = 0;
   total_probe_pos = 0;
   total_surf_points = 0;
   for (iatom = 0; iatom < natoms; iatom++) {
      if (surface_request[iatom]) {

         /*
          * Determine the number of neighbors to this atom.
          */
   
         num_neighbor = 0;
         for (jatom = 0; jatom < natoms; jatom++) {
            if ((jatom != iatom) && surface_request[jatom]) {
               d1 = atom_radii[iatom] + atom_radii[jatom] 
                  + 2.0 * probe_radius;
               d1 *= d1;
               if (jatom < iatom) {
                  if (d_matrix[CONNOLLYoffset(natoms,jatom,iatom)] < d1) {
                     neighbor_list[num_neighbor++] = jatom;
                  }
               } else {
                  if (d_matrix[CONNOLLYoffset(natoms,iatom,jatom)] < d1) {
                     neighbor_list[num_neighbor++] = jatom;
                  }
               }
            }
         }

         /*
          * Generate reentrant surface.
          */

         if (probe_radius != 0.0) {
            if (!CONNOLLYdo_reentrant ( iatom, natoms, 
                                        surface_request, atom_radii, 
                                        (float (*)[3])atom_coords,
                                        atom_index,
                                        probe_radius, num_points_on_probe, 
                                        density_of_points, 
                                        (float (*)[3])probe_points,
                                        num_circumference,
                                        (float (*)[3])circle,
                                        num_neighbor, neighbor_list, 
                                        mutual_neighbor_list,
                                        d_matrix, 
                                        ayon, arcs,
                                        &total_probe_pos, &total_surf_points,
                                        &total_yon_probes, 
                                        &current_sp_allocated,
                                        probe_pos, &surf_points, 
                                        yon_probes )) {
               return (0);
             }
         } /* end if (probe_radius != 0.0) */

         if (surface_request[iatom] == 2) {
            if (!CONNOLLYgen_contact ( iatom, atom_radii, 
                                       (float (*)[3])atom_coords, 
                                       atom_index,
                                       probe_radius, density_of_points,
                                       (float (*)[3])contact_points,
                                       num_neighbor, neighbor_list,
                                       &total_surf_points, 
                                       &current_sp_allocated, 
                                       &surf_points )) {
               return (0);
            }
         } /* end if (surface_request[iatom] == 2) */
      } /* end if (surface_request[iatom]) */
   }

   if (probe_radius != 0.0) {
      CONNOLLYremove_overlaps ( probe_radius,
                                total_probe_pos, total_yon_probes,
                                probe_pos, surf_points, 
                                &yon_probes[0], &yon_probes[num_probe_pos],
                                &yon_probes[2*num_probe_pos] );
      CONNOLLYsort_surf_points ( &total_surf_points, surf_points );
   } /* end if (probe_radius != 0.0) */

   delete [] d_matrix;
   delete [] neighbor_list;
   delete [] contact_points;
   if (probe_radius != 0.0) {
      delete mutual_neighbor_list;
      delete ayon;
      delete [] probe_points;
      delete [] circle;
      delete [] probe_pos;
      delete [] yon_probes;
      delete [] arcs;
   }

   *total_returned = total_surf_points;
   *the_surface = surf_points;
   return (1);
}

int
CONNOLLYdo_reentrant(int32_t iatom, int32_t natoms, int8_t *surface_request,
    float *atom_radii, float atom_coords[][3], int32_t *atom_index,
    float probe_radius, int32_t num_points_on_probe, float density_of_points,
    float probe_points[][3], int32_t num_circumference, float circle[][3],
    int32_t num_neighbor, int32_t *neighbor_list,
    RWBitVec *mutual_neighbor_list,
    float *d_matrix, RWBitVec *ayon, float *arcs, int32_t *total_probe_pos,
    int32_t *total_surf_points, int32_t *total_yon_probes,
    int32_t *current_sp_allocated, 
    probe_descriptor *probe_pos, sp_descriptor **surf_points,
    int32_t *yon_probes)
{
   int32_t intersect;
   float d1;

   int32_t jnbr;
   int32_t knbr;

   int32_t pair[2];
   float height;
   float alt[3];
   float alt2[2][3];
   float base[3];

   int32_t surface_i;
   int32_t index_i;
   float radius_i;
   float coord_i[3];

   int32_t jatom;
   int32_t index_j;
   int32_t surface_j;
   float radius_j;
   float coord_j[3];

   int32_t katom;
   int32_t index_k;
   int32_t surface_k;
   float radius_k;
   float coord_k[3];

   float uij1[3];
   float uij2[3];
   float pij[3];

   float cw[2][3];

   /*
    */

   surface_i = 0;
   if (surface_request[iatom] == 2) surface_i = 1;
   radius_i = atom_radii[iatom];
   coord_i[0] = atom_coords[iatom][0];
   coord_i[1] = atom_coords[iatom][1];
   coord_i[2] = atom_coords[iatom][2];
   index_i = atom_index[iatom];

   for (jnbr = 0; jnbr < num_neighbor; jnbr++) {
      jatom = neighbor_list[jnbr];
      if (jatom > iatom) {
         radius_j = atom_radii[jatom];
         surface_j = 0;
         if (surface_request[jatom] == 2) {
            surface_j = 1;
         }
         coord_j[0] = atom_coords[jatom][0];
         coord_j[1] = atom_coords[jatom][1];
         coord_j[2] = atom_coords[jatom][2];
         index_j = atom_index[jatom];
   
         uij1[0] = coord_j[0] - coord_i[0];
         uij1[1] = coord_j[1] - coord_i[1];
         uij1[2] = coord_j[2] - coord_i[2];
   
         /*
          * uij1 is the difference between the 2 atoms' XYZ coordinates
          * and uij2 is a unit vector along the inter-atomic axis.
          * pij is a unit vector perpendicular to uij2.
          */
   
         CONNOLLYvnorm ( uij1, uij2 );
         CONNOLLYvperp ( uij2, pij );
   
         /*
          * Find the circle of intersection of the expanded spheres
          * of the 2 atoms.  The "expanded sphere" is the sum of the
          * atom's radius and the probe's radius.  This circle of
          * intersection will be known as the "saddle circle".
          * From what I can determine from Connolly's paper, this
          * saddle circle is also the circle formed when rotating
          * about the iatom-jatom axis at the "base" point32_t with the
          * radius of the circle being the "alt".  (see below)
          *
          * For saddle-shaped reentrant surfaces and 
          * concave reentrant surfaces
          * we are working with a plane passing through the
          * circle of intersection of the expanded spheres of
          * iatom and jatom and also with a line perpendicular
          * to this plane passing through the centers of iatom
          * and jatom.  The point32_t at which the line and the plane
          * intersect is the variable "base".  The variable "alt"
          * is a vector from "base" to the center of the probe
          * sphere which is simultaneously touching iatom and jatom.
          * The plane will be known as the "saddle plane".
          */
   
         CONNOLLYcircle_circle_intersection ( coord_i, coord_j, 
            radius_i+probe_radius, radius_j+probe_radius, 
            pij, base, alt, &intersect );
   
         if (intersect) {
            height = CONNOLLYanorm ( alt );
   
            /*
             * Loop over the neighbor list to determine possible mutual
             * neighbors.
             */
   
            for (knbr = 0; knbr < num_neighbor; knbr++) {
               katom = neighbor_list[knbr];
               mutual_neighbor_list->clearBit(knbr);
               if (knbr != jnbr) {
                  d1 = 2.0 * probe_radius + radius_j + atom_radii[katom];
                  d1 *= d1;
                  if (jatom < katom) {
                     if (d_matrix[CONNOLLYoffset(natoms, jatom, katom)] < d1) {
                        mutual_neighbor_list->setBit(knbr);
                     }
                  } else {
                     if (d_matrix[CONNOLLYoffset(natoms, katom, jatom)] < d1) {
                        mutual_neighbor_list->setBit(knbr);
                     }
                  }
               }
            }
   
            /*
             * Loop over neighbor list and see if we should do
             * a concave reentrant surface.
             */
   
            for (knbr = 0; knbr < num_neighbor; knbr++) {
               katom = neighbor_list[knbr];
      
               if (CONNOLLYdo_concave ( jnbr, knbr,
                                        iatom, jatom, katom,
                                        num_neighbor, neighbor_list,
                                        mutual_neighbor_list,
                                        surface_request, atom_radii, 
                                        atom_coords,
                                        probe_radius, height, base, uij2,
                                        pair, cw, alt2 )) {
                  /*
                   * Generate surface points for the concave reentrant
                   * surface.
                   */
   
                  radius_k = atom_radii[katom];
                  coord_k[0] = atom_coords[katom][0];
                  coord_k[1] = atom_coords[katom][1];
                  coord_k[2] = atom_coords[katom][2];
                  index_k = atom_index[katom];
                  surface_k = 0;
                  if (surface_request[katom] == 2) surface_k = 1;
                  if (!CONNOLLYgen_concave ( probe_radius, 
                                             num_points_on_probe, 
                                             probe_points,
                                             pair, cw, alt2,
                                             index_i, surface_i, 
                                             radius_i, coord_i,
                                             index_j, surface_j, 
                                             radius_j, coord_j,
                                             index_k, surface_k, 
                                             radius_k, coord_k,
                                             total_probe_pos, 
                                             total_surf_points,
                                             total_yon_probes, 
                                             current_sp_allocated,
                                             probe_pos, surf_points, 
                                             yon_probes )) {
                     return (0);
                  }
               } /* end if (do_concave */
            } /* end for (knbr = 0; knbr < num_neighbor; knbr++) */
   
            if (surface_i || surface_j) {
               if(!CONNOLLYgen_saddle ( jnbr, iatom, jatom,
                                        num_neighbor, neighbor_list,
                                        surface_request, atom_radii, 
                                        atom_coords,
                                        atom_index,
                                        probe_radius, density_of_points, 
                                        num_circumference, circle,
                                        height, alt, base, uij2,
                                        ayon, &arcs[0], 
                                        (float (*)[3])&arcs[num_circumference],
                                        (float (*)[3])&arcs[num_circumference*3 
                                            + num_circumference],
                                        total_probe_pos, total_surf_points,
                                        total_yon_probes, current_sp_allocated,
                                        probe_pos, surf_points, yon_probes )) {
                  return (0);
               }
            } /* end if (surface_i && surface_j) */
         } /* end if (intersect) */
      } /* end if (jatom > iatom) */
   } /* end for (jnbr = 0; jnbr < num_neighbor; jnbr++) */
   return (1);
}

/*
 * For a concave reentrant surface the probe touches 3 atoms and thus
 * has no degrees of freedom.  That is, there are only 2 positions in
 * which the probe can touch 3 atoms: one above the plane defined by
 * the atom positions and one below the plane.  As in the saddle
 * reentrant surface, when the probe touches 2 atoms an arc is formed
 * from the point32_t of contact on one atom to the point32_t of contact on
 * the second atom.  This radius of curvature of this arc is that of
 * the probe sphere.  For concave reentrant surfaces there are 3 such
 * arcs (since there are 3 pairs of atoms) which define a spherical
 * triangle.
 */

int
CONNOLLYdo_concave(int32_t jnbr, int32_t knbr,
    int32_t iatom, int32_t jatom, int32_t katom,
    int32_t num_neighbor, int32_t *neighbor_list, RWBitVec *mutual_neighbor,
    int8_t *surface_request, float *atom_radii, float atom_coords[][3],
    float probe_radius, float height, float base[3], float uij2[3],
    int32_t pair[2], float cw[2][3], float alt2[2][3])
{
   int32_t i;
   int32_t intersect;
   float base2[3];
   float pbk;
   float rck;
   float cck[3];

   /*
    */

   if (!mutual_neighbor->testBit(knbr)) return (0);
   if (katom <= jatom) return (0);

   if (surface_request[iatom] != 2 && surface_request[jatom] != 2
      && surface_request[katom] != 2) return (0);

   pbk = uij2[0] * (atom_coords[katom][0] - base[0])
       + uij2[1] * (atom_coords[katom][1] - base[1])
       + uij2[2] * (atom_coords[katom][2] - base[2]);
   rck = ((atom_radii[katom] + probe_radius) 
       *  (atom_radii[katom] + probe_radius)) - (pbk * pbk);
   if (rck <= 0.0) return (0);
   rck = sqrt(rck);

   /*
    * Geometric calculations for placing a probe
    * next to 3 atoms.  We are working with a plane
    * passing through 3 atoms and also with a line
    * perpendicular to this plane passing through
    * the center of the probe sphere.  The point
    * where the line and the plane intersect is the
    * variable "base2".  The variable "alt2" is a
    * pair of vectors from the base to the center
    * of the probe spheres which would simultaneously
    * touch the 3 atoms.  These points are on opposite
    * sides of the plane of the 3 atoms.  The "alt2"
    * positions are where the "saddle circle" intersects
    * the "expanded sphere" of katom.
    *
    * pbk will be the projection of a vector that goes
    * from "base" to the center of katom onto the the
    * line connecting iatom and jatom.  (uij2 is a unit
    * vector connecting iatom and jatom).  pbk is the
    * distance from the center of katom to the "saddle
    * plane".  If this distance is larger than the "expanded
    * sphere" of katom then we have no intersection.  (This
    * is the calculation of "rck" above).  "rck" will be
    * the radius of the circle generated by the intersection
    * of the "expanded sphere" of katom with the "saddle plane".
    * The variable "cck" is the center of this circle.
    */

   for (i = 0; i < 3; i++) {
      cck[i] = atom_coords[katom][i] - pbk*uij2[i];
   }

   /*
    * Determine where the circle generated by the intersection of
    * the "expanded sphere" of katom with the "saddle plane" intersects
    * the "saddle circle".  "cck" is the center of the first circle with
    * radius "rck" while "base" is the center of the "saddle circle" with
    * radius "height".
    */

   CONNOLLYcircle_circle_intersection ( cck, base, rck,
                                        height, uij2, 
                                        base2, alt2[0], &intersect );
   if (!intersect) return (0);

   /*
    * circle_circle_intersection returns one altitude vector.  The
    * second vector is just the negative of the first.
    */

   for (i = 0; i < 3; i++) {
      alt2[1][i] = -alt2[0][i];
   }

   /*
    * The 2 possible probe positions (one above the plane and one below)
    * will be stored in "cw".
    */

   for (i = 0; i < 3; i++) {
      cw[0][i] = base2[i] + alt2[0][i];
      cw[1][i] = base2[i] + alt2[1][i];
   }


   /*
    * Check collision of the probes with neighboring atoms.
    */

   pair[0] = 1 - CONNOLLYcollide ( jnbr, knbr,
                                   num_neighbor, neighbor_list, 
                                   mutual_neighbor,
                                   atom_radii, atom_coords, 3, 
                                   probe_radius, cw[0] );
   pair[1] = 1 - CONNOLLYcollide ( jnbr, knbr,
                                   num_neighbor, neighbor_list, 
                                   mutual_neighbor,
                                   atom_radii, atom_coords, 3, 
                                   probe_radius, cw[1] );

   if ((pair[0] == 0) && (pair[1] == 0)) return (0);
   return (1);
}

/*
 * For a concave reentrant surface the probe touches 3 atoms and thus
 * has no degrees of freedom.  That is, there are only 2 positions in
 * which the probe can touch 3 atoms: one above the plane defined by
 * the atom positions and one below the plane.  As in the saddle
 * reentrant surface, when the probe touches 2 atoms an arc is formed
 * from the point32_t of contact on one atom to the point32_t of contact on
 * the second atom.  This radius of curvature of this arc is that of
 * the probe sphere.  For concave reentrant surfaces there are 3 such
 * arcs (since there are 3 pairs of atoms) which define a spherical
 * triangle.
 *
 * The do_concave function calculated the positions of the 2 probe points
 * which are in "cw".
 */

int
CONNOLLYgen_concave(float probe_radius, int32_t num_points_on_probe,
    float probe_points[][3], int32_t pair[2], float cw[2][3],
    float alt2[2][3], int32_t index_i, int32_t surface_i, float radius_i,
    float coord_i[3], int32_t index_j, int32_t surface_j, float radius_j,
    float coord_j[3], int32_t index_k, int32_t surface_k, float radius_k,
    float coord_k[3], int32_t *total_probe_pos, int32_t *total_surf_points,
    int32_t *total_yon_probes, int32_t *current_sp_allocated,
    probe_descriptor *probe_pos, sp_descriptor **surf_points,
    int32_t *yon_probes)
{
   int32_t any_yon;
   int32_t i;
   int32_t j;
   int32_t k;
   int32_t both;
   int32_t np;
   int32_t tmp_total_pp;
   int32_t tmp_total_sp;
   int32_t tmp_total_yp;
   int32_t local_sp_alloc;
   int32_t yon;
   float area;
   float di;
   float dj;
   float dk;
   float probe_radius_2;
   float sign;
   float wi[3];
   float wj[3];
   float wk[3];
   float sco[3];
   sp_descriptor *local_surf_points;

   /*
    */

   tmp_total_pp = *total_probe_pos;
   tmp_total_sp = *total_surf_points;
   tmp_total_yp = *total_yon_probes;
   local_sp_alloc = *current_sp_allocated;
   local_surf_points = *surf_points;

   probe_radius_2 = probe_radius * probe_radius;
   both = 0;
   if (pair[0] && pair[1]) both = 1;

   area = (4.0 * M_PI * probe_radius_2) / num_points_on_probe;
   for (i = 0; i < 2; i++) {
      if (pair[i]) {

         /*
          * Calculate vectors defining spherical triangle.
          * (These are not the same definitions as given in
          * the paper).
          */

         for (j = 0; j < 3; j++) {
            wi[j] = coord_i[j] - cw[i][j];
            wj[j] = coord_j[j] - cw[i][j];
            wk[j] = coord_k[j] - cw[i][j];
         }
         sign = CONNOLLYdet ( wi, wj, wk );
         np = 0;

         /*
          * Gather points on probe sphere lying within triangle.
          * We generate a set of unit vectors (we use those calculated
          * at the start of the program).  A vector lies within the
          * triangle if:
          * (wi _cross_ wj _dot_ wk), 
          * (unit_vector _cross_ wj _dot_ wk), 
          * (wi _cross_ unit_vector _dot_ wk) and
          * (wi _cross_ wj _dot_ unit_vector) all have the same sign.
          * yon keeps track of whether the surface point32_t we have calculated
          * is on the same side or opposite side of the symmetry plane.
          */

         while ((tmp_total_sp + num_points_on_probe) > local_sp_alloc) {
            local_sp_alloc = local_sp_alloc + SP_ALLOC_INC;
            *current_sp_allocated = local_sp_alloc;
            if (!(local_surf_points = (sp_descriptor *)
               realloc ( *surf_points, 
                         local_sp_alloc * sizeof(sp_descriptor) ))) {
               error_return ( "%s",
                          "Could not realloc space for SURF POINTS." );
               return (0);
            } else {
               *surf_points = local_surf_points;
            }
         }

         probe_pos[tmp_total_pp].start_point = tmp_total_sp;
         any_yon = 0;
         for (j = 0; j < num_points_on_probe; j++) {
            if ( (sign * CONNOLLYdet ( probe_points[j], wj, wk ) >= 0.0)
              && (sign * CONNOLLYdet ( wi, probe_points[j], wk ) >= 0.0)
              && (sign * CONNOLLYdet ( wi, wj, probe_points[j] ) >= 0.0) ) {
               yon = 0;
               if ((alt2[i][0] * (alt2[i][0] + probe_points[j][0]) +
                    alt2[i][1] * (alt2[i][1] + probe_points[j][1]) +
                    alt2[i][2] * (alt2[i][2] + probe_points[j][2]))
                    < 0.0 ) {
                    yon = 1;
                    any_yon = 1;
               }

               /*
                * If the point32_t is not on the opposite side of the
                * symmetry plane or if both probes are not involved
                * in collisions then we calculate a surface point
                * position.
                */

               if (!yon || !both) {
                  for (k = 0; k < 3; k++) {
                     sco[k] = cw[i][k] + probe_points[j][k]
                            * probe_radius;
                  }

                  di = CONNOLLYdist ( sco, coord_i ) - radius_i;
                  dj = CONNOLLYdist ( sco, coord_j ) - radius_j;
                  dk = CONNOLLYdist ( sco, coord_k ) - radius_k;

                  if ((di <= dj) && (di <= dk)) {
                     if (surface_i) {
                        local_surf_points[tmp_total_sp].atom1 = index_i;
                        local_surf_points[tmp_total_sp].atom2 = index_j;
                        local_surf_points[tmp_total_sp].atom3 = index_k;
                        local_surf_points[tmp_total_sp].shape = 3;
                        local_surf_points[tmp_total_sp].coord[0] = sco[0];
                        local_surf_points[tmp_total_sp].coord[1] = sco[1];
                        local_surf_points[tmp_total_sp].coord[2] = sco[2];
                        local_surf_points[tmp_total_sp].area = area;
                        tmp_total_sp++;
                        np++;
                     }
                  } else if ((dj <= di) && (dj <= dk)) {
                     if (surface_j) {
                        local_surf_points[tmp_total_sp].atom1 = index_j;
                        local_surf_points[tmp_total_sp].atom2 = index_i;
                        local_surf_points[tmp_total_sp].atom3 = index_k;
                        local_surf_points[tmp_total_sp].shape = 3;
                        local_surf_points[tmp_total_sp].coord[0] = sco[0];
                        local_surf_points[tmp_total_sp].coord[1] = sco[1];
                        local_surf_points[tmp_total_sp].coord[2] = sco[2];
                        local_surf_points[tmp_total_sp].area = area;
                        tmp_total_sp++;
                        np++;
                     }
                  } else {
                     if (surface_k) {
                        local_surf_points[tmp_total_sp].atom1 = index_k;
                        local_surf_points[tmp_total_sp].atom2 = index_i;
                        local_surf_points[tmp_total_sp].atom3 = index_j;
                        local_surf_points[tmp_total_sp].shape = 3;
                        local_surf_points[tmp_total_sp].coord[0] = sco[0];
                        local_surf_points[tmp_total_sp].coord[1] = sco[1];
                        local_surf_points[tmp_total_sp].coord[2] = sco[2];
                        local_surf_points[tmp_total_sp].area = area;
                        tmp_total_sp++;
                        np++;
                     }
                  }
               }
            }
         } /* end for (j = 0; j < num_points_on_probe; j++) */

         if (np != 0) {
            probe_pos[tmp_total_pp].shape = 3;
            probe_pos[tmp_total_pp].num_points = np;
            probe_pos[tmp_total_pp].coord[0] = cw[i][0];
            probe_pos[tmp_total_pp].coord[1] = cw[i][1];
            probe_pos[tmp_total_pp].coord[2] = cw[i][2];
            probe_pos[tmp_total_pp].alt[0] = alt2[i][0];
            probe_pos[tmp_total_pp].alt[1] = alt2[i][1];
            probe_pos[tmp_total_pp].alt[2] = alt2[i][2];
            probe_pos[tmp_total_pp].yon = any_yon;
            probe_pos[tmp_total_pp].victim = 0;

            if (any_yon) {
               yon_probes[tmp_total_yp++] = tmp_total_pp;
            }
            tmp_total_pp++;

         } /* end if (np != 0) */
      } /* end if (pair[i])  */
   } /* end for (i = 0; i < 2; i++) */
   *total_probe_pos = tmp_total_pp;
   *total_surf_points = tmp_total_sp;
   *total_yon_probes = tmp_total_yp;
   return (1);
}

int
CONNOLLYgen_saddle(int32_t jnbr, int32_t iatom, int32_t jatom, int32_t num_neighbor,
    int32_t *neighbor_list, int8_t *surface_request, float *atom_radii,
    float atom_coords[][3], int32_t *atom_index, float probe_radius,
    float density_of_points, int32_t num_circumference, float circle[][3],
    float height, float alt[3], float base[3], float uij2[3],
    RWBitVec *ayon, float *arca, float arc0[][3], float arc1[][3],
    int32_t *total_probe_pos, int32_t *total_surf_points, int32_t *total_yon_probes,
    int32_t *current_sp_allocated, probe_descriptor *probe_pos, 
    sp_descriptor **surf_points, int32_t *yon_probes)
{
   int32_t any_yon;
   int32_t both;
   int32_t i;
   int32_t k;
   int32_t irot;
   int32_t iw;
   int32_t np;
   int32_t num_arcs;
   int32_t num_rotations;
   int32_t pair[2];
   int32_t tmp_total_pp;
   int32_t tmp_total_sp;
   int32_t tmp_total_yp;
   int32_t local_sp_alloc;
   float angle;
   float average;
   float di;
   float dj;
   float p;
   float ra;
   float rij;
   float arct[3];
   float arcr[3];
   float vector[3];
   float wi[3];
   float wj[3];
   float cw[2][3];
   float ralt[2][3];
   float g[3][3];
   float ghgt[3][3];
   float h[3][3];
   float power[3][3];
   float sco[3];

   sp_descriptor *local_surf_points;

   /*
    * The radial vector from the "base" to one of the probe points
    * is a function of the angle of rotationa about the iatom-jatom
    * axis.  This angle is incremented in discrete steps and the
    * angular increment must be chosen so that the distance traveled
    * by a point32_t on the arc from one probe position to the next is
    * 1 / sqrt (density_of_points), but this distance depends on how
    * far the point32_t is from the iatom-jatom axis.  The 2 end points
    * of the arc are given by 
    * (height * radius_of_iatom / (radius_of_iatom + probe_radius)) and
    * (height * radius_of_jatom / (radius_of_jatom + probe_radius)).
    * The bottom point32_t of the arc is at a distance of
    * abs ( height - probe_radius )
    * from the iatom-jatom axis.  The average of these 3 distances is
    * used to find the number of rotations and the rotation angle
    * is given by 
    * (I * M_PI / number_of_rotations)
    * where I goes from 0 to number_of_rotations.
    */

   local_surf_points = *surf_points;
   local_sp_alloc = *current_sp_allocated;

   rij = atom_radii[iatom] / (atom_radii[iatom] + probe_radius)
       + atom_radii[jatom] / (atom_radii[jatom] + probe_radius);
   average = (abs(height - probe_radius) + height * rij) / 3.0;
   // cast to int32_t added by fabien fontaine the 14/12/2000
   num_rotations = (int32_t) (sqrt(density_of_points) * M_PI * average);
   if (num_rotations < 1) num_rotations = 1;
   angle = M_PI / num_rotations;

   CONNOLLYidentity_matrix ( h );
   h[1][1] = cos ( angle );
   h[2][2] = h[1][1];
   h[1][2] = sin ( angle );
   h[2][1] = -h[1][2];

   /*
    * We now want to calculate surface points.  We will start with
    * a circle of unit vectors in the XY plane and later rotate this
    * circle onto the "saddle plane".
    *
    * Calculate matrix to rotate X-axis onto the iatom-jatom axis.
    */

   for (k = 0; k < 3; k++) {
      g[k][0] = 0.0;
      g[k][1] = 0.0;
      g[k][2] = 0.0;
   }
   for (k = 0; k < 3; k++) {
      g[0][k] = uij2[k];
      g[1][k] = alt[k] / height;
   }
   CONNOLLYcross ( g[0], g[1], g[2] );

   /*
    * We use 2 probes 180 degrees from each other in order to facilitate
    * overlapping surface point32_t removal latter on.
    *
    * Make the probe pair rotation matrix be about the iatom-jatom axis.
    */

   CONNOLLYconjugate ( h, g, ghgt );

   /*
    * Arc generation.
    * cw will hold the probe position and wi and wj are vectors from
    * iatom and jatom to the probe position.
    */

   for (k = 0; k < 3; k++) {
      cw[0][k] = base[k] + alt[k];
      wi[k] = atom_coords[iatom][k] - cw[0][k];
      wj[k] = atom_coords[jatom][k] - cw[0][k];
   }

   /*
    * Rotate circle onto iatom-jatom-water plane and select points
    * between water-iatom and water-jatom vector to form the arc.
    * Only those points which satisfy:
    * (wi _cross_ surface_point) _dot_ rotation_matrix > 0.0 and
    * (wj _cross_ surface_point) _dot_ rotation_matrix > 0.0 are
    * along the arc between the 2 points of contact.
    */

   num_arcs = 0;
   for (i = 0; i < num_circumference; i++) {
      CONNOLLYmultv ( circle[i], g, arct );
      CONNOLLYcross ( wi, arct, vector );
      if (CONNOLLYdot ( g[2], vector ) >= 0.0) {
         CONNOLLYcross ( arct, wj, vector );
         if (CONNOLLYdot ( g[2], vector ) >= 0.0) {

            for (k = 0; k < 3; k++) {
               arc0[num_arcs][k] = arct[k] + alt[k];
            }

            /*
             * Invert arc through line of symmetry.
             */

            p = CONNOLLYdot ( uij2, arc0[num_arcs] );
            for (k = 0; k < 3; k++) {
               arc1[num_arcs][k] = -arc0[num_arcs][k] + 2.0 * p * uij2[k];
            }

            /*
             * Check whether the arc point32_t crosses the iatom-jatom axis
             * and calculate the area associated with the point.
             */

            ra = CONNOLLYdot ( alt, arc0[num_arcs] );
            ayon->clearBit(num_arcs);
            if (ra < 0.0) ayon->setBit(num_arcs);
            ra = abs(ra / height);
            arca[num_arcs] = (2.0 * M_PI * M_PI * probe_radius * ra)
                           / (num_circumference * num_rotations);
            num_arcs++;
         } /* end if (dot ( g[2], vector ) >= 0.0) */
      } /* end if (dot ( g[2], vector ) >= 0.0) */
   } /* end for (i = 0; i < num_circumference; i++) */

   /*
    * Initialize power matrix to indentity.
    */

   CONNOLLYidentity_matrix ( power );

   /*
    * Rotate the probe pair around the pair of atoms.
    */

   tmp_total_pp = *total_probe_pos;
   tmp_total_sp = *total_surf_points;
   tmp_total_yp = *total_yon_probes;

   for (irot = 0; irot < num_rotations; irot++) {
      CONNOLLYmultv ( alt, power, ralt[0] );
      for (k = 0; k < 3; k++) {
         ralt[1][k] = -ralt[0][k];
      }

      for (iw = 0; iw < 2; iw++) {
         for (k = 0; k < 3; k++) {
            cw[iw][k] = base[k] + ralt[iw][k];
         }

         /*
          * Check for collisions with neighboring atoms.
          */

         pair[iw] = 1 - CONNOLLYcollide ( jnbr, 0, num_neighbor, 
                                          neighbor_list, NULL,
                                          atom_radii, atom_coords, 2, 
                                          probe_radius, cw[iw] );
      } /* end for (iw = 0; iw < 2; iw++) */

      if (pair[0] || pair[1]) {
         both = 0;
         if (pair[0] && pair[1]) both = 1;

         for (iw = 0; iw < 2; iw++) {
            if (pair[iw]) {
               probe_pos[tmp_total_pp].start_point = tmp_total_sp;
               any_yon = 0;
               np = 0;

               while ((tmp_total_sp + num_arcs) > local_sp_alloc) {
                  local_sp_alloc = local_sp_alloc + SP_ALLOC_INC;
                  *current_sp_allocated = local_sp_alloc;
                  if (!(local_surf_points = (sp_descriptor *)
                     realloc ( *surf_points, 
                              local_sp_alloc * sizeof(sp_descriptor) ))) {
                     error_return ( "%s",
                                "Could not realloc SURF POINTS." );
                     return (0);
                  } else {
                     *surf_points = local_surf_points;
                  }
               }

               /*
                * The saddle-shaped reentrant surface points come from
                * the arc.
                */

               for (i = 0; i < num_arcs; i++) {

                  /*
                   * Overlapping reentrant surface removal for
                   * symmetry-related probe positions.
                   */

                  if (!both || !ayon->testBit(i)) {

                     if (ayon->testBit(i)) any_yon = 1;
                     /*
                      * Rotate the arc from the XY plane onto the
                      * iatom-jatom-probe plane.
                      */

                     if (iw == 0) CONNOLLYmultv ( arc0[i], power, arcr );
                     if (iw == 1) CONNOLLYmultv ( arc1[i], power, arcr );

                     /*
                      * Make coordinates relative to the origin.
                      */

                     for (k = 0; k < 3; k++) {
                        sco[k] = base[k] + arcr[k];
                     }

                     /*
                      * Find the closest atom.
                      */

                     di = CONNOLLYdist ( sco, atom_coords[iatom] ) 
                        - atom_radii[iatom];
                     dj = CONNOLLYdist ( sco, atom_coords[jatom] ) 
                        - atom_radii[jatom];

                     if (di <= dj) {
                        if (surface_request[iatom] == 2) {
                           local_surf_points[tmp_total_sp].atom1 
                              = atom_index[iatom];
                           local_surf_points[tmp_total_sp].atom2 
                              = atom_index[jatom];
                           local_surf_points[tmp_total_sp].atom3 = -1;
                           local_surf_points[tmp_total_sp].shape = 2;
                           local_surf_points[tmp_total_sp].coord[0] = sco[0];
                           local_surf_points[tmp_total_sp].coord[1] = sco[1];
                           local_surf_points[tmp_total_sp].coord[2] = sco[2];
                           local_surf_points[tmp_total_sp].area = arca[i];
                           tmp_total_sp++;
                           np++;
                        }
                     } else {
                        if (surface_request[jatom] == 2) {
                           local_surf_points[tmp_total_sp].atom1 
                              = atom_index[jatom];
                           local_surf_points[tmp_total_sp].atom2 
                              = atom_index[iatom];
                           local_surf_points[tmp_total_sp].atom3 = -1;
                           local_surf_points[tmp_total_sp].shape = 2;
                           local_surf_points[tmp_total_sp].coord[0] = sco[0];
                           local_surf_points[tmp_total_sp].coord[1] = sco[1];
                           local_surf_points[tmp_total_sp].coord[2] = sco[2];
                           local_surf_points[tmp_total_sp].area = arca[i];
                           tmp_total_sp++;
                           np++;
                        }
                     }
                  } /* end if (!both || !ayon->testBit(i)) */
               } /* end for (i = 0; i < num_arcs; i++) */


               probe_pos[tmp_total_pp].shape = 2;
               probe_pos[tmp_total_pp].num_points = np;
               probe_pos[tmp_total_pp].coord[0] = cw[iw][0];
               probe_pos[tmp_total_pp].coord[1] = cw[iw][1];
               probe_pos[tmp_total_pp].coord[2] = cw[iw][2];
               probe_pos[tmp_total_pp].alt[0] = ralt[iw][0];
               probe_pos[tmp_total_pp].alt[1] = ralt[iw][1];
               probe_pos[tmp_total_pp].alt[2] = ralt[iw][2];
               probe_pos[tmp_total_pp].yon = any_yon;
               probe_pos[tmp_total_pp].victim = 0;

               if (any_yon) {
                  yon_probes[tmp_total_yp++] = tmp_total_pp;
               }

               if (np != 0) {
                  tmp_total_pp++;
               }
            } /* end if (pair[iw]) */
         } /* end for (iw = 0; iw < 2; iw++) */
      } /* if (pair[0] || pair[1]) */

      /*
       * Calculate a new power matrix.
       */

      CONNOLLYconcat ( power, ghgt );
   } /* for (irot = 0; irot < num_rotations; irot++) */

   *total_probe_pos = tmp_total_pp;
   *total_surf_points = tmp_total_sp;
   *total_yon_probes = tmp_total_yp;
   return (1);
}

int
CONNOLLYgen_contact(int32_t iatom, float *atom_radii,
    float atom_coords[][3], int32_t *atom_index, float probe_radius,
    float density_of_points, float contact_points[][3], int32_t num_neighbor,
    int32_t *neighbor_list, int32_t *total_surf_points, int32_t *current_sp_allocated,
    sp_descriptor **surf_points)
{
   int32_t i;
   int32_t k;
   int32_t num_contacts;
   float area;
   float radius_i;
   float cw[3];

   int32_t tmp_total_sp;
   int32_t local_sp_alloc;
   sp_descriptor *local_surf_points;

   /*
    */

   radius_i = atom_radii[iatom];
   // cast to int32_t added by fabien fontaine the 14/12/2000
   num_contacts = (int32_t) (4.0 * M_PI * radius_i * radius_i * density_of_points);
   CONNOLLYgen_unit_vectors ( contact_points, &num_contacts );
   area = 4.0 * M_PI * radius_i * radius_i / num_contacts;

   tmp_total_sp = *total_surf_points;
   local_sp_alloc = *current_sp_allocated;
   local_surf_points = *surf_points;

   while ((tmp_total_sp + num_contacts) > local_sp_alloc) {
      local_sp_alloc = local_sp_alloc + SP_ALLOC_INC;
      *current_sp_allocated = local_sp_alloc;
      if (!(local_surf_points = (sp_descriptor *)
         realloc ( *surf_points, local_sp_alloc * sizeof(sp_descriptor) ))) {
         error_return ( "%s",
                    "Could not realloc space for SURF POINTS." );
         return (0);
      } else {
         *surf_points = local_surf_points;
      }
   }

   for (i = 0; i < num_contacts; i++) {
      for (k = 0; k < 3; k++) {
         cw[k] = atom_coords[iatom][k] + (radius_i + probe_radius)
               * contact_points[i][k];
      }

      if (!CONNOLLYcollide ( 0, 0, num_neighbor, neighbor_list, NULL,
                             atom_radii, atom_coords, 1, probe_radius, cw )) {
         local_surf_points[tmp_total_sp].atom1 = atom_index[iatom];
         local_surf_points[tmp_total_sp].atom2 = -1;
         local_surf_points[tmp_total_sp].atom3 = -1;
         local_surf_points[tmp_total_sp].shape = 1;
         local_surf_points[tmp_total_sp].coord[0] = 
            atom_coords[iatom][0] + radius_i*contact_points[i][0];
         local_surf_points[tmp_total_sp].coord[1] = 
            atom_coords[iatom][1] + radius_i*contact_points[i][1];
         local_surf_points[tmp_total_sp].coord[2] = 
            atom_coords[iatom][2] + radius_i*contact_points[i][2];
         local_surf_points[tmp_total_sp].area = area;
         local_surf_points[tmp_total_sp].norm[0] = contact_points[i][0];
         local_surf_points[tmp_total_sp].norm[1] = contact_points[i][1];
         local_surf_points[tmp_total_sp].norm[2] = contact_points[i][2];
         tmp_total_sp++;
      }
   }
   *total_surf_points = tmp_total_sp;
   return (1);
}

void
CONNOLLYremove_overlaps(float probe_radius, int32_t total_probe_pos,
    int32_t total_yon_probes, probe_descriptor *probe_pos,
    sp_descriptor *surf_points, int32_t *yon_probes, int32_t *victim_probes,
    int32_t *eater_probes)
{
   int32_t do_print;
   int32_t i;
   int32_t j;
   int32_t k;
   int32_t num_eaters;
   int32_t total_victim_probes;
   float d2;
   float dw;
   float dw2;
   float rw2;
   float xx;
   float yy;
   float zz;
   float alt[3];
   float pos[3];
   float yon[3];
   float victim[3];
   float yon_alt[3];
   float victim_alt[3];

   /*
    * Connolly's first pass at reading the file just generated a list
    * of which probe positions had any yon surface_points.  We have
    * already determined this as the surface points were generated.
    *
    * If there are no yon probes then just write out the points.
    */

   if (total_yon_probes == 0) {
      CONNOLLYwrite_all ( probe_radius, total_probe_pos, 
                  probe_pos, surf_points );
      return;
   }

   /*
    * His second pass went through all of the probe positions to see
    * if there was any overlap with any of the yon probe positions.
    * These were labeled victims.
    */

   dw = 2.0 * probe_radius;
   dw2 = dw * dw;
   rw2 = probe_radius * probe_radius;
   total_victim_probes = 0;

   for (i = 0; i < total_probe_pos; i++) {
      if (!probe_pos[i].yon) {
         victim[0] = probe_pos[i].coord[0];
         victim[1] = probe_pos[i].coord[1];
         victim[2] = probe_pos[i].coord[2];
         victim_alt[0] = probe_pos[i].alt[0];
         victim_alt[1] = probe_pos[i].alt[1];
         victim_alt[2] = probe_pos[i].alt[2];
         for (j = 0; j < total_yon_probes; j++) {
            yon[0] = probe_pos[yon_probes[j]].coord[0];
            yon[1] = probe_pos[yon_probes[j]].coord[1];
            yon[2] = probe_pos[yon_probes[j]].coord[2];
            yon_alt[0] = probe_pos[yon_probes[j]].alt[0];
            yon_alt[1] = probe_pos[yon_probes[j]].alt[1];
            yon_alt[2] = probe_pos[yon_probes[j]].alt[2];

            xx = abs(yon[0] - victim[0]);
            if (xx < dw) {
               yy = abs(yon[1] - victim[1]);
               if (yy < dw) {
                  zz = abs(yon[2] - victim[2]);
                  if (zz < dw) {
                     d2 = xx*xx + yy*yy + zz*zz;
                     if (d2 < dw2) {
                        if (CONNOLLYdot ( yon_alt, victim_alt ) < 0.0) {
                           probe_pos[i].victim = 1;
                           victim_probes[total_victim_probes++] = i;
                           break;
                        }
                     }
                  }
               }
            }
         }
      }
   }

   for (i = 0; i < total_probe_pos; i++) {
      num_eaters = 0;
      pos[0] = probe_pos[i].coord[0];
      pos[1] = probe_pos[i].coord[1];
      pos[2] = probe_pos[i].coord[2];
      alt[0] = probe_pos[i].alt[0];
      alt[1] = probe_pos[i].alt[1];
      alt[2] = probe_pos[i].alt[2];

      /*
       * If this is not a yon or victim probe point32_t the write out it's
       * surface points.
       */

      if (!probe_pos[i].yon && !probe_pos[i].victim) {
         for (j = probe_pos[i].start_point; 
              j < probe_pos[i].start_point + probe_pos[i].num_points; j++) {
            surf_points[j].norm[0] = 
               (pos[0] - surf_points[j].coord[0]) / probe_radius;
            surf_points[j].norm[1] = 
               (pos[1] - surf_points[j].coord[1]) / probe_radius;
            surf_points[j].norm[2] = 
               (pos[2] - surf_points[j].coord[2]) / probe_radius;
         } /* end for (j = probe_pos[i].start_point */

      /*
       * If this is a victim or a yon probe then check against all yon
       * probes.
       */

      } else {
         for (j = 0; j < total_yon_probes; j++) {
            yon[0] = probe_pos[yon_probes[j]].coord[0];
            yon[1] = probe_pos[yon_probes[j]].coord[1];
            yon[2] = probe_pos[yon_probes[j]].coord[2];
            yon_alt[0] = probe_pos[yon_probes[j]].alt[0];
            yon_alt[1] = probe_pos[yon_probes[j]].alt[1];
            yon_alt[2] = probe_pos[yon_probes[j]].alt[2];

            if (CONNOLLYdist2 ( pos, yon ) < dw2) {
               if (CONNOLLYdot ( alt, yon_alt ) < 0.0) {
                  eater_probes[num_eaters++] = yon_probes[j];
               } /* if (CONNOLLYdot ( alt, yon_alt ) < 0.0) */
            } /* end if (CONNOLLYdist2 ( pos, yon ) < dw2) */
         } /* end for (j = 0; j < total_yon_probes; j++) */

         /*
          * If this is a yon probe point32_t then check against all yon probes
          * and all victim probes.
          */

         if (probe_pos[i].yon) {
            for (j = 0; j < total_victim_probes; j++) {
               victim[0] = probe_pos[victim_probes[j]].coord[0];
               victim[1] = probe_pos[victim_probes[j]].coord[1];
               victim[2] = probe_pos[victim_probes[j]].coord[2];
               victim_alt[0] = probe_pos[victim_probes[j]].alt[0];
               victim_alt[1] = probe_pos[victim_probes[j]].alt[1];
               victim_alt[2] = probe_pos[victim_probes[j]].alt[2];
   
               if (CONNOLLYdist2 ( pos, yon ) < dw2) {
                  if (CONNOLLYdot ( alt, victim_alt ) < 0.0) {
                     eater_probes[num_eaters++] = victim_probes[j];
                  } /* if (CONNOLLYdot ( alt, yon_alt ) < 0.0) */
               } /* end if (CONNOLLYdist2 ( pos, yon ) < dw2) */
            } /* end for (j = 0; j < total_victim_probes; j++) */
         } /* end if (probe_pos[i].yon) */

         /*
          * Now eat.
          */

         if (num_eaters == 0) {
            for (j = probe_pos[i].start_point;
                 j < probe_pos[i].start_point + probe_pos[i].num_points; j++) {

               surf_points[j].norm[0] = 
                  (pos[0] - surf_points[j].coord[0]) / probe_radius;
               surf_points[j].norm[1] = 
                  (pos[1] - surf_points[j].coord[1]) / probe_radius;
               surf_points[j].norm[2] = 
                  (pos[2] - surf_points[j].coord[2]) / probe_radius;
            } /* for (j = probe_pos[i].start_point; */
         } else {
            for (j = probe_pos[i].start_point;
                 j < probe_pos[i].start_point + probe_pos[i].num_points; j++) {

               do_print = 1;
               for (k = 0; k < num_eaters; k++) {
                  if (CONNOLLYdist2 ( probe_pos[eater_probes[k]].coord, 
                              surf_points[j].coord ) < rw2) {
                     do_print = 0;
                     break;
                  } /* end if (CONNOLLYdist2 ( probe_pos[eater_probes[k]].coord, */
               } /* end for (k = 0; k < num_eaters; k++) */
               if (do_print) {
                  surf_points[j].norm[0] = 
                     (pos[0] - surf_points[j].coord[0]) / probe_radius;
                  surf_points[j].norm[1] = 
                     (pos[1] - surf_points[j].coord[1]) / probe_radius;
                  surf_points[j].norm[2] = 
                     (pos[2] - surf_points[j].coord[2]) / probe_radius;
               } else {
                  surf_points[j].atom1 = 999999;
               } /* end if (do_print) */
            } /* end for (j = probe_pos[i].start_point; */
         } /* end if (num_eaters == 0) */
      } /* end if (!probe_pos[i].yon && !probe_pos[i].victim) */
   } /* end for (i = 0; i < total_probe_pos; i++) */
}

void
CONNOLLYcircle_circle_intersection(float a[3], float b[3], float ra,
    float rb, float perpendicular[3], float base[3], float altitude[3],
    int32_t *intersect)
{
   int32_t k;

   float c1;
   float dab;
   float v;
   float alt[3];
   float c[3];
   float c1v[3];
   float cu[3];

   /*
    */

   for (k = 0; k < 3; k++) {
      c[k] = b[k] - a[k];
   }
   dab = CONNOLLYanorm ( c );

   *intersect = 1;

   /*
    * See if we have a triangle.
    * dab is the distance between a and b.
    */

   if ((ra + rb) <= dab) *intersect = 0;
   if ((ra + dab) <= rb) *intersect = 0;
   if ((dab + rb) <= ra) *intersect = 0;
   if (*intersect == 0) return;

   CONNOLLYvnorm ( c, cu );
   c1 = 0.5 * (dab + (ra*ra - rb*rb)/dab);
   for (k = 0; k < 3; k++) {
      c1v[k] = c[k] * (c1 / dab);
      base[k] = a[k] + c1v[k];
   }

   v = sqrt (ra*ra - c1*c1);
   CONNOLLYcross ( c, perpendicular, altitude );
   CONNOLLYvnorm ( altitude, alt );
   for (k = 0; k < 3; k++) {
      altitude[k] = alt[k] * v;
   }
}

#define THRESHOLD -0.0000001

/*
 * Collision check of probe with neighboring atoms.
 * The checks are as follows:
 * 1) If "shape" > 1 (which means we are dealing with a reentrant surface)
 *    and the neighbor we are looking up in the atom list is the current
 *    neighbor atom (jatom) then ignore the check.  (Look into this as
 *    our neighbor list might not contain jatom so there is no need for this
 *    check).
 * 2) If the "shape" is a concave reentrant surface and i is katom or i
 *    is not a mutual neighbor then ignore the check.
 * 3) sum_radius is the sum of the current i atom plus the probe_radius
 * 4) The original then did the following:
 *    a) if abs(cw[0] - neighbor[0]) > sum_radius then skip to next atom
 *    b) if abs(cw[1] - neighbor[1]) > sum_radius then skip to next atom
 *    c) if abs(cw[2] - neighbor[2]) > sum_radius then skip to next atom
 *    d) square the differences from a, b, c and compare with 
 *       sum_radius squared.
 *    e) return true if they collide, else continue looping
 */

int
CONNOLLYcollide(int32_t jnbr, int32_t knbr, int32_t num_neighbors,
    int32_t *neighbor_list, RWBitVec *mutual_neighbor, float *atom_radii,
    float atom_coords[][3], int32_t shape, float probe_radius, float cw[3])
{
   int32_t inbr;
   int32_t iatom;
   float d2;
   float sum_radius;
   float vect_x;
   float vect_y;
   float vect_z;

   /*
    */

   if (num_neighbors == 0) return (0);

   if (shape == 3) {
      for (inbr = 0; inbr < num_neighbors; inbr++) {
         if (inbr != jnbr) {
            if ((inbr != knbr) && mutual_neighbor->testBit(inbr)) {
               iatom = neighbor_list[inbr];
               sum_radius = atom_radii[iatom] + probe_radius;
               vect_x = abs(cw[0] - atom_coords[iatom][0]);
               if ((vect_x - sum_radius) < THRESHOLD) {
                  vect_y = abs(cw[1] - atom_coords[iatom][1]);
                  if ((vect_y - sum_radius) < THRESHOLD) {
                     vect_z = abs(cw[2] - atom_coords[iatom][2]);
                     if ((vect_z - sum_radius) < THRESHOLD) {
                        sum_radius *= sum_radius;
                        d2 = vect_x*vect_x + vect_y*vect_y + vect_z*vect_z;
                        if ((d2 - sum_radius) < THRESHOLD) return (1);
                     }
                  }
               }
            }
         }
      }
      return (0);
   } else if (shape == 2) {
      for (inbr = 0; inbr < num_neighbors; inbr++) {
         if (inbr != jnbr) {
            iatom = neighbor_list[inbr];
            sum_radius = atom_radii[iatom] + probe_radius;
            vect_x = abs(cw[0] - atom_coords[iatom][0]);
            if ((vect_x - sum_radius) < THRESHOLD) {
               vect_y = abs(cw[1] - atom_coords[iatom][1]);
               if ((vect_y - sum_radius) < THRESHOLD) {
                  vect_z = abs(cw[2] - atom_coords[iatom][2]);
                  if ((vect_z - sum_radius) < THRESHOLD) {
                     sum_radius *= sum_radius;
                     d2 = vect_x*vect_x + vect_y*vect_y + vect_z*vect_z;
                     if ((d2 - sum_radius) < THRESHOLD) return (1);
                  }
               }
            }
         }
      }
      return (0);
   } else if (shape == 1) {
      for (inbr = 0; inbr < num_neighbors; inbr++) {
         iatom = neighbor_list[inbr];
         sum_radius = atom_radii[iatom] + probe_radius;
         vect_x = abs(cw[0] - atom_coords[iatom][0]);
         if ((vect_x - sum_radius) < THRESHOLD) {
            vect_y = abs(cw[1] - atom_coords[iatom][1]);
            if ((vect_y - sum_radius) < THRESHOLD) {
               vect_z = abs(cw[2] - atom_coords[iatom][2]);
               if ((vect_z - sum_radius) < THRESHOLD) {
                  sum_radius *= sum_radius;
                  d2 = vect_x*vect_x + vect_y*vect_y + vect_z*vect_z;
                  if ((d2 - sum_radius) < THRESHOLD) return (1);
               }
            }
         }
      }
      return (0);
   }
   return (0);
}

#undef THRESHOLD

void
CONNOLLYwrite_all(float probe_radius, int32_t total_probe_pos,  
    probe_descriptor *probe_pos, sp_descriptor *surf_points)
{
   int32_t i;
   int32_t j;

   for (i = 0; i < total_probe_pos; i++) {
      for (j = probe_pos[i].start_point;
           j < probe_pos[i].start_point + probe_pos[i].num_points; j++) {

         surf_points[j].norm[0] = 
            (probe_pos[i].coord[0] - surf_points[j].coord[0]) / probe_radius;
         surf_points[j].norm[1] = 
            (probe_pos[i].coord[1] - surf_points[j].coord[1]) / probe_radius;
         surf_points[j].norm[2] = 
            (probe_pos[i].coord[2] - surf_points[j].coord[2]) / probe_radius;
      }
   }
}

/*
 * Generate unit vectors U(tn) = (sin(t)cos(n), sin(t)sin(n)cos(t)) where
 * t = I * M_PI / m, 0 <= i <= m and n = I * 2*M_PI/N(t), 0 <= i < N(t).
 *
 * num_equators = 2*M_PI*probe_radius * sqrt(density_of_points).
 * num_vert  = M_PI * probe_radius * sqrt(density_of_points).
 * These values are chosen so as to give an psacing between points of
 * approximately 1 / sqrt(density_of_points).
 *
 * num_equators is the circumference (2 * M_PI * R) times sqrt(denisty_of_points)
 * therefore it is the number of circumferences to be placed around the
 * sphere.
 */

void
CONNOLLYgen_unit_vectors(float probe_points[][3], int32_t *num_points_on_probe)
{
   int32_t i;
   int32_t j;
   int32_t num_unit_vectors;
   int32_t nhor;
   int32_t n;
   int32_t num_equators;
   int32_t num_vert;
   float fi;
   float fj;
   float x;
   float y;
   float z;
   float xy;

   n = *num_points_on_probe;
   // cast to int32_t added by fabien fontaine the 14/12/2000
   num_equators = (int32_t) sqrt ( n * M_PI );
   num_vert = (int32_t) (0.5 * num_equators);
   /* This next line may not be correct.  May want to just return. */
   if (num_vert < 1) num_vert = 1;
   num_unit_vectors = 0;

   for (i = 0; i <= num_vert; i++) {
      fi = (M_PI * i) / num_vert;
      z = cos(fi);
      xy = sin(fi);
      // cast to int32_t added by fabien fontaine the 14/12/2000
      nhor = (int32_t) (num_equators * xy);
      if (nhor < 1) nhor = 1;
      for (j = 0; j <= nhor-1; j++) {
         fj = (2 * M_PI * j) / nhor;
         x = cos(fj) * xy;
         y = sin(fj) * xy;
         if (num_unit_vectors < n) {
            probe_points[num_unit_vectors][0] = x;
            probe_points[num_unit_vectors][1] = y;
            probe_points[num_unit_vectors][2] = z;
            num_unit_vectors++;
         }
      }
   }
   *num_points_on_probe = num_unit_vectors;
}

/*
 * Return offset into the distance_matrix given natoms, iatom and jatom
 * (n, i, j).
 */

int
CONNOLLYoffset(int32_t n, int32_t i, int32_t j)
{
   int32_t ni;

   ni = n - i;
   return ( n*(n-1)/2 - ni*(ni-1)/2 + j - i - 1 );
}

/*
 * Compute the distance between 2 points.
 */

float
CONNOLLYdist(float a[3], float b[3])
{
   float dx;
   float dy;
   float dz;

   dx = a[0] - b[0];
   dx *= dx;
   dy = a[1] - b[1];
   dy *= dy;
   dz = a[2] - b[2];
   dz *= dz;

   return ( sqrt(dx + dy + dz) );
}

/*
 * Compute the square of the distance between 2 points.
 */

float
CONNOLLYdist2(float a[3], float b[3])
{
   float dx;
   float dy;
   float dz;

   dx = a[0] - b[0];
   dx *= dx;
   dy = a[1] - b[1];
   dy *= dy;
   dz = a[2] - b[2];
   dz *= dz;

   return ( (dx + dy + dz) );
}

/*
 * Return the norm of A.
 */

float 
CONNOLLYanorm(float a[3])
{
   return ( sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]) );
}

/*
 * Return the dot product of A and B.
 */

float
CONNOLLYdot(float a[3], float b[3])
{
   return ( a[0]*b[0] + a[1]*b[1] + a[2]*b[2] );
}

/*
 * Return the cross product of A and B in C.
 */

void
CONNOLLYcross(float a[3], float b[3], float c[3])
{
   c[0] = a[1]*b[2] - a[2]*b[1];
   c[1] = a[2]*b[0] - a[0]*b[2];
   c[2] = a[0]*b[1] - a[1]*b[0];
}

/*
 * Multiply V by A giving W.
 */

void
CONNOLLYmultv(float v[3], float a[3][3], float w[3])
{
   int32_t i;

   for (i = 0; i < 3; i++) {
      w[i] = a[0][i]*v[0] + a[1][i]*v[1] + a[2][i]*v[2];
   }
}

/*
 * Normalize A giving B.
 */

void
CONNOLLYvnorm(float a[3], float b[3])
{
   int32_t i;
   float v;

   v = CONNOLLYanorm ( a );
   for (i = 0; i < 3; i++) {
      b[i] = a[i] / v;
   }
}

/*
 * Return B perpendicular to A.
 */

// --> compatibility issues
#ifdef small
#define baksmall small
#undef small
#endif
// <-- compatibility issues

void
CONNOLLYvperp(float a[3], float b[3])
{
   int32_t k;
   int32_t m;
   float dt;
   float small;
   float p[3];

   small = 10000.0;
   m = -1;
   for (k = 0; k < 3; k++) {
      if (abs(a[k]) < small) {
         small = abs(a[k]);
         m = k;
      }
   }

   for (k = 0; k < 3; k++) {
      b[k] = 0.0;
      if (k == m) b[m] = 1.0;
   }

   dt = a[m] / (a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
   for (k = 0; k < 3; k++) {
      p[k] = dt * a[k];
      b[k] = b[k] - p[k];
   }

   CONNOLLYvnorm ( b, p );
   for (k = 0; k < 3; k++) {
      b[k] = p[k];
   }
}

/*
 * Return triple product of A, B, C
 */

float
CONNOLLYdet(float a[3], float b[3], float c[3])
{
   float ab[3];

   CONNOLLYcross ( a, b, ab );
   return (CONNOLLYdot ( ab, c ));
}

/*
 * Concatentate matrix B onto matrix A.
 */

void
CONNOLLYconcat(float a[3][3], float b[3][3])
{
   int32_t i;
   int32_t j;
   float temp[3][3];

   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
         temp[j][i] = a[0][i]*b[j][0] + a[1][i]*b[j][1] + a[2][i]*b[j][2];
      }
   }
   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
         a[j][i] = temp[j][i];
      }
   }
}

/*
 * Conjugate matrix G with matrix H giving GHGT.
 */

void
CONNOLLYconjugate(float h[3][3], float g[3][3], float ghgt[3][3])
{
   int32_t k;
   int32_t l;
   float gt[3][3];

   CONNOLLYidentity_matrix ( ghgt );
   CONNOLLYconcat ( ghgt, g );
   CONNOLLYconcat ( ghgt, h );
   for (k = 0; k < 3; k++) {
      for (l = 0; l < 3; l++) {
         gt[l][k] = g[k][l];
      }
   }
   CONNOLLYconcat ( ghgt, gt );
}

/*
 * Load identity matrix.
 */

void
CONNOLLYidentity_matrix(float a[3][3])
{
   int32_t i;
   int32_t j;

   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
         a[i][j] = 0.0;
      }
      a[i][i] = 1.0;
   }
}

void
error_return ( const char *format, const char *args )
{
   char str[1024];

   sprintf ( str, format, args );

   // GDB 3/4/12  0 arg added to eliminate compiler warning
   fprintf(stderr, str, 0);
   exit ( 0 );
}

void
CONNOLLYsort_surf_points(int32_t *total_surf_points, sp_descriptor *surf_points)
{
   int32_t i;
   int32_t local_total_sp;
   int32_t tmp_total_sp;

   local_total_sp = *total_surf_points;
   tmp_total_sp = *total_surf_points;

   qsort ( (void *)surf_points, local_total_sp, sizeof(sp_descriptor), 
           CONNOLLYcompar );

   for (i = 0; i < local_total_sp; i++) {
      if (surf_points[i].atom1 == 999999) tmp_total_sp--;
   }
   *total_surf_points = tmp_total_sp;
}

int
CONNOLLYcompar(const void *spt1, const void *spt2)
{
   int32_t i;

   sp_descriptor *sp1 = (sp_descriptor *)spt1;
   sp_descriptor *sp2 = (sp_descriptor *)spt2;

   if ((i = CONNOLLYint_compar ( sp1->atom1, sp2->atom1 )) != 0) return (i);
   if ((i = CONNOLLYint_compar ( sp1->atom2, sp2->atom2 )) != 0) return (i);
   if ((i = CONNOLLYint_compar ( sp1->atom3, sp2->atom3 )) != 0) return (i);
   if ((i = CONNOLLYint_compar ( sp1->shape, sp2->shape )) != 0) return (i);
   if ((i = CONNOLLYfloat_compar ( sp1->coord[0], sp2->coord[0] )) != 0) 
      return (i);
   if ((i = CONNOLLYfloat_compar ( sp1->coord[1], sp2->coord[1] )) != 0) 
      return (i);
   if ((i = CONNOLLYfloat_compar ( sp1->coord[2], sp2->coord[2] )) != 0) 
      return (i);
   if ((i = CONNOLLYfloat_compar ( sp1->area, sp2->area )) != 0) return (i);
   if ((i = CONNOLLYfloat_compar ( sp1->norm[0], sp2->norm[0] )) != 0) 
      return (i);
   if ((i = CONNOLLYfloat_compar ( sp1->norm[1], sp2->norm[1] )) != 0) 
      return (i);
   if ((i = CONNOLLYfloat_compar ( sp1->norm[2], sp2->norm[2] )) != 0) 
      return (i);
   return (0);
}

int
CONNOLLYint_compar(int32_t int1, int32_t int2)
{
   if (int1 < int2) return (-1);
   if (int1 == int2) return (0);
   if (int1 > int2) return (1);
   return 0;
}

int
CONNOLLYfloat_compar(float float1, float float2)
{
   if (float1 < float2) return (-1);
   if (float1 == float2) return (0);
   if (float1 > float2) return (1);
   return 0;
}

#undef abs
#undef min
#undef max

// --> compatibility issues
#ifdef backsmall
#define small backsmall
#undef backsmall
#endif
// <-- compatibility issues
