//  Per-pixel lighting for ChemDisplay's spheres and cylinders (#210); see
//  inv/ChemKit/ChemPerPixel.H.
#include "inv/ChemKit/ChemPerPixel.H"

#ifdef OIV_COIN

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <sstream>
#include <string>

#include <Inventor/misc/SoState.h>
#include <Inventor/misc/SoContextHandler.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/C/glue/gl.h>
#include <Inventor/system/gl.h>      // the platform's GL header, as Coin finds it

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_CURRENT_PROGRAM 0x8B8D
#endif

namespace {

typedef unsigned int (APIENTRY *CreateShaderFn)(unsigned int);
typedef void (APIENTRY *ShaderSourceFn)(unsigned int, int, const char *const *, const int *);
typedef void (APIENTRY *CompileShaderFn)(unsigned int);
typedef void (APIENTRY *GetivFn)(unsigned int, unsigned int, int *);
typedef void (APIENTRY *GetLogFn)(unsigned int, int, int *, char *);
typedef unsigned int (APIENTRY *CreateProgramFn)(void);
typedef void (APIENTRY *AttachShaderFn)(unsigned int, unsigned int);
typedef void (APIENTRY *LinkProgramFn)(unsigned int);
typedef void (APIENTRY *UseProgramFn)(unsigned int);
typedef void (APIENTRY *DeleteShaderFn)(unsigned int);

//  ftransform() keeps depth identical to the fixed-function shapes drawn
//  with the atoms (labels, isosurfaces, wireframe).
const char *vertexSource =
	"#version 120\n"
	"varying vec3 ecPos;\n"
	"varying vec3 ecNormal;\n"
	"void main() {\n"
	"  vec4 ec = gl_ModelViewMatrix * gl_Vertex;\n"
	"  ecPos = ec.xyz / ec.w;\n"
	"  ecNormal = gl_NormalMatrix * gl_Normal;\n"
	"  gl_FrontColor = gl_Color;\n"
	"  gl_BackColor = gl_Color;\n"
	"  gl_ClipVertex = ec;\n"
	"  gl_FogFragCoord = abs(ecPos.z);\n"
	"  gl_Position = ftransform();\n"
	"}\n";

//  The GL state a fragment program is specialised for.  One program per
//  combination keeps the shader to the lights actually on (usually just
//  the headlight): a loop over all eight with uniform switches cost a
//  third more frame time on software GL.
struct Key {
	unsigned lights = 0;        // bit i: GL_LIGHTi enabled
	unsigned positional = 0;    // bit i: GL_LIGHTi has w != 0
	int fog = 0;                // 0 off, 1 linear, 2 exp, 3 exp2
	bool localViewer = false, twoSide = false;
	bool operator<(const Key &o) const {
		if (lights != o.lights) return lights < o.lights;
		if (positional != o.positional) return positional < o.positional;
		if (fog != o.fog) return fog < o.fog;
		if (localViewer != o.localViewer) return localViewer < o.localViewer;
		return twoSide < o.twoSide;
	}
};

//  The fixed-function lighting equation (GL 2.1 section 2.14.1) per
//  fragment, with the diffuse from glColor as GL_COLOR_MATERIAL(GL_DIFFUSE)
//  takes it.
std::string fragmentSource(const Key &k)
{
	std::ostringstream s;
	s << "#version 120\n"
	     "varying vec3 ecPos;\n"
	     "varying vec3 ecNormal;\n"
	     "void main() {\n"
	     "  vec3 n = normalize(ecNormal);\n";
	if (k.twoSide) s << "  if (!gl_FrontFacing) n = -n;\n";
	s << "  vec3 v = " << (k.localViewer ? "normalize(-ecPos)" : "vec3(0.0, 0.0, 1.0)")
	  << ";\n"
	     "  vec4 diffuse = gl_Color;\n"
	     "  vec3 c = gl_FrontMaterial.emission.rgb\n"
	     "         + gl_FrontMaterial.ambient.rgb * gl_LightModel.ambient.rgb;\n"
	     "  vec3 l; float att, nl;\n";
	for (int i = 0; i < 8; i++) {
		if (!(k.lights & (1u << i))) continue;
		std::string L = "gl_LightSource[" + std::to_string(i) + "]";
		if (k.positional & (1u << i)) {
			s << "  { vec3 d = " << L << ".position.xyz - ecPos;\n"
			     "    float dist = length(d);\n"
			     "    l = d / dist;\n"
			     "    att = 1.0 / (" << L << ".constantAttenuation + "
			  << L << ".linearAttenuation * dist + "
			  << L << ".quadraticAttenuation * dist * dist);\n"
			     "    if (" << L << ".spotCutoff <= 90.0) {\n"
			     "      float sp = dot(-l, normalize(" << L << ".spotDirection));\n"
			     "      att *= sp < " << L << ".spotCosCutoff ? 0.0 : pow(max(sp, 0.0), "
			  << L << ".spotExponent);\n"
			     "    } }\n";
		} else {
			s << "  l = normalize(" << L << ".position.xyz); att = 1.0;\n";
		}
		s << "  nl = max(dot(n, l), 0.0);\n"
		     "  c += att * (gl_FrontMaterial.ambient.rgb * " << L << ".ambient.rgb\n"
		     "       + nl * diffuse.rgb * " << L << ".diffuse.rgb);\n"
		     "  if (nl > 0.0) c += att * pow(max(dot(n, normalize(l + v)), 0.0),"
		     " gl_FrontMaterial.shininess)\n"
		     "       * gl_FrontMaterial.specular.rgb * " << L << ".specular.rgb;\n";
	}
	s << "  vec4 color = vec4(clamp(c, 0.0, 1.0), diffuse.a);\n";
	if (k.fog) {
		s << "  float z = gl_FogFragCoord;\n";
		if (k.fog == 1) s << "  float f = (gl_Fog.end - z) * gl_Fog.scale;\n";
		else if (k.fog == 2) s << "  float f = exp(-gl_Fog.density * z);\n";
		else s << "  float f = exp(-pow(gl_Fog.density * z, 2.0));\n";
		s << "  color.rgb = mix(gl_Fog.color.rgb, color.rgb, clamp(f, 0.0, 1.0));\n";
	}
	s << "  gl_FragColor = color;\n"
	     "}\n";
	return s.str();
}

struct Context {
	bool failed = false;
	UseProgramFn use = 0;
	std::map<Key, unsigned int> programs;
};

std::map<int, Context> contexts;   // by Coin cache context id
const char *lastStatus = "off";
unsigned int previous = 0;
Context *bound = 0;

void sayOnce(const std::string &why)
{
	static bool said = false;
	if (said) return;
	said = true;
	fprintf(stderr, "ECCE viewer: per-pixel lighting %s\n", why.c_str());
}

void unavailable(Context &c, const std::string &why)
{
	c.failed = true;
	sayOnce("unavailable (" + why + "); using fixed-function lighting");
}

bool disabledByEnv()
{
	const char *e = getenv("ECCE_GL_PERPIXEL");
	return e != 0 && strcmp(e, "0") == 0;
}

std::string trimmed(const char *log)
{
	std::string t(log);
	while (!t.empty() && (t.back() == '\n' || t.back() == ' ')) t.pop_back();
	return t;
}

#define PROC(type, name) type name = (type)cc_glglue_getprocaddress(glue, #name)

unsigned int compile(const cc_glglue *glue, unsigned int type,
	const std::string &src, std::string &log)
{
	PROC(CreateShaderFn, glCreateShader);
	PROC(ShaderSourceFn, glShaderSource);
	PROC(CompileShaderFn, glCompileShader);
	PROC(GetivFn, glGetShaderiv);
	PROC(GetLogFn, glGetShaderInfoLog);
	if (!glCreateShader || !glShaderSource || !glCompileShader ||
		!glGetShaderiv || !glGetShaderInfoLog) {
		log = "missing GL 2.0 shader entry points";
		return 0;
	}
	unsigned int s = glCreateShader(type);
	const char *text = src.c_str();
	glShaderSource(s, 1, &text, 0);
	glCompileShader(s);
	int ok = 0;
	glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char buf[2048] = {0};
		glGetShaderInfoLog(s, sizeof(buf) - 1, 0, buf);
		log = std::string("shader compile failed: ") + trimmed(buf);
		return 0;
	}
	return s;
}

//  The program for one state in one context, 0 (and c.failed) on failure.
unsigned int build(Context &c, const cc_glglue *glue, const Key &k)
{
	if (!cc_glglue_glversion_matches_at_least(glue, 2, 0, 0)) {
		unavailable(c, "GL older than 2.0");
		return 0;
	}
	PROC(CreateProgramFn, glCreateProgram);
	PROC(AttachShaderFn, glAttachShader);
	PROC(LinkProgramFn, glLinkProgram);
	PROC(GetivFn, glGetProgramiv);
	PROC(GetLogFn, glGetProgramInfoLog);
	PROC(DeleteShaderFn, glDeleteShader);
	c.use = (UseProgramFn)cc_glglue_getprocaddress(glue, "glUseProgram");
	if (!glCreateProgram || !glAttachShader || !glLinkProgram ||
		!glGetProgramiv || !glGetProgramInfoLog || !glDeleteShader || !c.use) {
		unavailable(c, "missing GL 2.0 entry points");
		return 0;
	}
	std::string log;
	unsigned int vs = compile(glue, GL_VERTEX_SHADER, vertexSource, log);
	unsigned int fs = vs ? compile(glue, GL_FRAGMENT_SHADER, fragmentSource(k), log) : 0;
	if (!fs) {
		unavailable(c, log);
		return 0;
	}
	unsigned int p = glCreateProgram();
	glAttachShader(p, vs);
	glAttachShader(p, fs);
	glLinkProgram(p);
	glDeleteShader(vs);
	glDeleteShader(fs);
	int ok = 0;
	glGetProgramiv(p, GL_LINK_STATUS, &ok);
	if (!ok) {
		char buf[2048] = {0};
		glGetProgramInfoLog(p, sizeof(buf) - 1, 0, buf);
		unavailable(c, std::string("link failed: ") + trimmed(buf));
		return 0;
	}
	// The normal case; every viewer process would otherwise print it.
	if (getenv("ECCE_DEBUG_GL")) sayOnce("on");
	return p;
}

//  Coin hands out a new id per context; forget its programs with it.
void contextGone(uint32_t ctx, void *)
{
	contexts.erase((int)ctx);
}

}  // namespace

bool
ChemPerPixel::begin(SoState *state)
{
	bound = 0;
	if (disabledByEnv()) {
		sayOnce("off (ECCE_GL_PERPIXEL=0)");
		lastStatus = "off";
		return false;
	}
	static bool registered = false;
	if (!registered) {
		SoContextHandler::addContextDestructionCallback(contextGone, 0);
		registered = true;
	}
	const int ctx = SoGLCacheContextElement::get(state);
	Context &c = contexts[ctx];
	if (c.failed) {
		lastStatus = "unavailable";
		return false;
	}

	Key k;
	for (int i = 0; i < 8; i++) {
		if (!glIsEnabled(GL_LIGHT0 + i)) continue;
		k.lights |= 1u << i;
		GLfloat pos[4];
		glGetLightfv(GL_LIGHT0 + i, GL_POSITION, pos);
		if (pos[3] != 0.0f) k.positional |= 1u << i;
	}
	if (glIsEnabled(GL_FOG)) {
		GLint mode = GL_EXP;
		glGetIntegerv(GL_FOG_MODE, &mode);
		k.fog = mode == GL_LINEAR ? 1 : (mode == GL_EXP ? 2 : 3);
	}
	GLint lv = 0, ts = 0;
	glGetIntegerv(GL_LIGHT_MODEL_LOCAL_VIEWER, &lv);
	glGetIntegerv(GL_LIGHT_MODEL_TWO_SIDE, &ts);
	k.localViewer = lv != 0;
	k.twoSide = ts != 0;

	std::map<Key, unsigned int>::iterator it = c.programs.find(k);
	unsigned int program = it != c.programs.end() ? it->second :
		(c.programs[k] = build(c, cc_glglue_instance(ctx), k));
	if (program == 0) {
		lastStatus = "unavailable";
		return false;
	}
	lastStatus = "on";
	GLint prev = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &prev);
	previous = (unsigned int)prev;
	c.use(program);
	bound = &c;
	return true;
}

void
ChemPerPixel::end()
{
	if (bound == 0) return;
	bound->use(previous);
	bound = 0;
}

const char *
ChemPerPixel::status()
{
	return lastStatus;
}

#endif  // OIV_COIN
