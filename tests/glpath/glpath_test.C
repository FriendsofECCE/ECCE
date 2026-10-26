// Which GL_RENDERER strings count as software, and how ECCE_GL_PATH and the
// preference override the detection (#566).
#include "util/GLPath.H"
#include <cstdio>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main()
{
  const char* software[] = {
      "llvmpipe (LLVM 19.1.7, 256 bits)", "softpipe", "Mesa DRI swrast",
      "SVGA3D; build: RELEASE; LLVM;", "virgl (NVIDIA GeForce RTX 3060)",
      "Microsoft Basic Render Driver", "GDI Generic", "Apple Software Renderer",
      "LLVMPIPE"};
  for (const char* r : software) CHECK(GLPath::isSoftwareRenderer(r));

  const char* hardware[] = {
      "NVIDIA GeForce RTX 3060/PCIe/SSE2", "AMD Radeon RX 6700 XT (radeonsi, navi22, LLVM 19.1.7, DRM 3.61, 6.12.0)",
      "Apple M2", "Mesa Intel(R) UHD Graphics 620 (KBL GT2)",
      "Intel(R) Iris(R) Xe Graphics", "", "ANGLE (NVIDIA)"};
  for (const char* r : hardware) CHECK(!GLPath::isSoftwareRenderer(r));
  CHECK(!GLPath::isSoftwareRenderer(0));

  GLPath::Choice c = GLPath::choose("llvmpipe", 0, false);
  CHECK(c.software && c.why == GLPath::DETECTED);
  c = GLPath::choose("NVIDIA GeForce", "", false);
  CHECK(!c.software && c.why == GLPath::DETECTED);
  c = GLPath::choose("llvmpipe", "hardware", false);
  CHECK(!c.software && c.why == GLPath::ENVIRONMENT);
  c = GLPath::choose("NVIDIA GeForce", "Software", false);
  CHECK(c.software && c.why == GLPath::ENVIRONMENT);
  c = GLPath::choose("NVIDIA GeForce", "software", true);
  CHECK(c.software && c.why == GLPath::PREFERENCE);
  c = GLPath::choose("llvmpipe", "bogus", false);
  CHECK(c.software && c.why == GLPath::DETECTED);

  CHECK(!GLPath::known() && !GLPath::software());
  printf(fails ? "FAILED\n" : "ok\n");
  return fails ? 1 : 0;
}
