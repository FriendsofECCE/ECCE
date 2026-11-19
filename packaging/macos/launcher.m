/* ECCE.app's CFBundleExecutable.  TCC names the responsible process after
   the bundle's executable; as a bash script that was /bin/bash, so the
   folder-access prompt (and NS*FolderUsageDescription) never applied to
   ECCE.  This starts Resources/ecce.sh as a child (not exec) and stays as the app.

   It runs an AppKit event loop only to receive Finder's open-document and
   reopen Apple events (#1069): files of the first launch go on the child's
   command line; later ones, and a click on ECCE.app while it runs, start
   ecce.sh again, which hands them to the running copy and exits. */
#import <AppKit/AppKit.h>
#include <mach-o/dyld.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
static volatile pid_t child;
static char script[PATH_MAX + 32];

static void forward(int sig) { if (child > 0) kill(child, sig); }

/* bash ecce.sh ARGS...; 0 on failure. */
static pid_t start(NSArray<NSString *> *args)
{
  char **av = calloc(args.count + 3, sizeof *av);
  av[0] = "/bin/bash";
  av[1] = script;
  for (NSUInteger i = 0; i < args.count; i++)
    av[i + 2] = strdup(args[i].fileSystemRepresentation);
  pid_t pid;
  int err = posix_spawn(&pid, "/bin/bash", NULL, NULL, av, environ);
  for (NSUInteger i = 0; i < args.count; i++) free(av[i + 2]);
  free(av);
  if (err) { fprintf(stderr, "ECCE: cannot start %s: %s\n", script, strerror(err)); return 0; }
  return pid;
}

static int status(int st) { return WIFEXITED(st) ? WEXITSTATUS(st) : 128 + WTERMSIG(st); }

/* Calls done(status) on the main queue when pid ends. */
static void watch(pid_t pid, void (^done)(int))
{
  dispatch_source_t s = dispatch_source_create(DISPATCH_SOURCE_TYPE_PROC, pid,
                                               DISPATCH_PROC_EXIT, dispatch_get_main_queue());
  dispatch_source_set_event_handler(s, ^{
    int st = 0;
    while (waitpid(pid, &st, 0) < 0 && errno == EINTR) {}
    dispatch_source_cancel(s);
    done(status(st));
  });
  dispatch_resume(s);
}

/* A second start: it hands over to the running copy and exits. */
static void again(NSArray<NSString *> *files)
{
  pid_t pid = start(files);
  if (pid) watch(pid, ^(int st) { (void)st; });
}

@interface Launcher : NSObject <NSApplicationDelegate>
@property (strong) NSMutableArray<NSString *> *args;
@property BOOL started;
@end

@implementation Launcher
- (void)application:(NSApplication *)app openFiles:(NSArray<NSString *> *)files
{
  if (!self.started) [self.args addObjectsFromArray:files];
  else again(files);
  [app replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
}

- (void)applicationDidFinishLaunching:(NSNotification *)note
{
  self.started = YES;
  pid_t pid = start(self.args);
  if (!pid) exit(127);
  child = pid;
  watch(pid, ^(int st) { exit(st); });
}

/* ECCE.app clicked while running: a second start raises the window. */
- (BOOL)applicationShouldHandleReopen:(NSApplication *)app hasVisibleWindows:(BOOL)flag
{
  again(@[]);
  return NO;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)app
{
  forward(SIGTERM);
  return NSTerminateNow;
}
@end

int main(int argc, char **argv) {
  char exe[PATH_MAX];
  uint32_t n = sizeof exe;
  if (_NSGetExecutablePath(exe, &n) != 0) {
    fprintf(stderr, "ECCE: cannot find its own path\n");
    return 127;
  }
  /* Contents/MacOS/ecce -> Contents/Resources/ecce.sh (a script in MacOS
     would be a code subcomponent that codesign wants signed). */
  /* The path as spelled, not realpath()ed: the tests compare it with the
     caller's spelling (/var vs /private/var). */
  char *slash = strrchr(exe, '/');
  if (slash) *slash = 0;
  snprintf(script, sizeof script, "%s/../Resources/ecce.sh", exe);

  struct sigaction sa;
  memset(&sa, 0, sizeof sa);
  sa.sa_handler = forward;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGHUP, &sa, NULL);

  @autoreleasepool {
    NSMutableArray<NSString *> *args = [NSMutableArray array];
    for (int i = 1; i < argc; i++) {
      if (!strncmp(argv[i], "-psn_", 5)) continue;   /* old LaunchServices */
      [args addObject:[NSString stringWithUTF8String:argv[i]]];
    }
    /* A command (ECCE_APP_RUN, the CI checks) runs without the event loop. */
    if (getenv("ECCE_APP_RUN") && args.count) {
      pid_t pid = start(args);
      if (!pid) return 127;
      child = pid;
      int st;
      while (waitpid(pid, &st, 0) < 0)
        if (errno != EINTR) return 127;
      return status(st);
    }
    /* Arguments are ecce's, never documents for AppKit to open. */
    [[NSUserDefaults standardUserDefaults]
        registerDefaults:@{@"NSTreatUnknownArgumentsAsOpen": @"NO"}];
    NSApplication *app = [NSApplication sharedApplication];
    /* The Organizer has the Dock icon and the menus; this has neither. */
    [app setActivationPolicy:NSApplicationActivationPolicyProhibited];
    Launcher *delegate = [Launcher new];
    delegate.args = args;
    app.delegate = delegate;
    [app run];
  }
  return 0;
}
