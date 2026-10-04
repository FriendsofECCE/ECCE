//  Coin build only: renders a small lit scene through SoOffscreenRenderer on
//  the EGL context of CoinEglOffscreen, with no wx and no X server needed.
//  Exit 0 and "OK" if the picture holds non-background pixels.
//  Run it with and without DISPLAY (tools/coin/compare.sh does neither;
//  see docs/claude/wx-viewer).
#include <cstdio>
#include <unistd.h>

#include <Inventor/SoDB.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SoOffscreenRenderer.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSphere.h>

#include "inv/SoWx/CoinEglOffscreen.H"

int main()
{
  if (!CoinEglOffscreen::install()) { fprintf(stderr, "no EGL\n"); return 1; }
  SoDB::init();
  SoSeparator *root = new SoSeparator;
  root->ref();
  SoPerspectiveCamera *cam = new SoPerspectiveCamera;
  root->addChild(cam);
  root->addChild(new SoDirectionalLight);
  SoMaterial *m = new SoMaterial;
  m->diffuseColor.setValue(1, 0, 0);
  root->addChild(m);
  root->addChild(new SoSphere);
  SbViewportRegion vp(64, 64);
  cam->viewAll(root, vp);
  SoOffscreenRenderer r(vp);
  r.setComponents(SoOffscreenRenderer::RGB);
  r.setBackgroundColor(SbColor(0, 0, 0));
  if (!r.render(root) || !r.getBuffer()) { fprintf(stderr, "render failed\n"); return 1; }
  const unsigned char *p = r.getBuffer();
  int red = 0;
  for (int i = 0; i < 64 * 64; i++)
    if (p[3 * i] > 80 && p[3 * i + 1] < 40) red++;
  printf("%s: %d red pixels of %d\n", red > 100 ? "OK" : "FAIL", red, 64 * 64);
  fflush(stdout);
  _exit(red > 100 ? 0 : 1);   // skip Mesa teardown
}
