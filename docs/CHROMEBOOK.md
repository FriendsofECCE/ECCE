# ECCE on a Chromebook (untested)

**These steps have not been tried on a Chromebook.** They follow from how
ChromeOS runs Linux programs and from what the Debian packages need. If you
try them, please report what happened in an
[issue](https://github.com/FriendsofECCE/ECCE/issues), whether it worked or
not.

ECCE runs on a Chromebook through ChromeOS's own Linux environment, which is
a Debian system in a container. The ECCE packages are built for Debian 13
("trixie"), so the container has to be Debian 13.

**What is needed**

- A Chromebook with an Intel or AMD processor. ECCE has no Linux package
  for ARM processors; the Terminal command `dpkg --print-architecture` must
  print `amd64`.
- A Chromebook you own. On a school- or company-managed Chromebook the
  Linux environment is often switched off by the administrator.
- About 10 GB free for the Linux environment.

## 1. Turn on Linux

Open **Settings**, search for **Linux**, and choose **Linux development
environment › Set up** (on older ChromeOS versions: Settings › Advanced ›
Developers). Give it at least 10 GB of disk. When it is ready, the
**Terminal** app opens.

## 2. Check the Debian version

In the Terminal:

```
cat /etc/debian_version
```

If it starts with `13`, go on to step 4. If it starts with `12`, do step 3
first.

## 3. Debian 12 to Debian 13

Either of these. A) is cleaner; B) keeps the files you already have in the
Linux environment.

**A) A new Debian 13 container**, if your ChromeOS offers it. Open
`chrome://flags` in the browser, search for **Debian version for new
Crostini containers**, choose trixie and restart. Then remove the Linux
environment (Settings › Linux development environment › Remove; this
deletes everything in it) and set it up again as in step 1.

**B) Upgrade in place.** In the Terminal:

```
grep -rl bookworm /etc/apt/sources.list /etc/apt/sources.list.d/
```

lists the files that name Debian 12. Change `bookworm` to `trixie` in the
Debian ones (`deb.debian.org`, `security.debian.org`), for example

```
sudo sed -i 's/bookworm/trixie/g' /etc/apt/sources.list
```

and leave the ChromeOS integration file (the one naming
`storage.googleapis.com/cros-packages`) as it is. Then

```
sudo apt update
sudo apt full-upgrade
```

and restart the Linux environment (right-click Terminal in the shelf ›
Shut down Linux, then open Terminal again). `cat /etc/debian_version`
should now start with `13`.

## 4. Install ECCE

Download the two Debian packages of the newest release, `ecce-client_…_amd64.deb`
and `ecce-server_…_amd64.deb`, from the
[releases page](https://github.com/FriendsofECCE/ECCE/releases). Files
downloaded in ChromeOS are in the Downloads folder; move them to
**Linux files** in the Files app, so that the Terminal can see them. Then,
in the Terminal:

```
sudo apt install ./ecce-client_*_amd64.deb ./ecce-server_*_amd64.deb
```

The `./` matters: it tells apt these are local files, and apt then installs
what they depend on.

## 5. Start ECCE

In the Terminal, type `ecce`. The first time, choose **Store data on this
computer**, then **Continue**. ECCE-QM, the quantum chemistry program that
comes with ECCE, runs inside the Linux environment; other codes are
installed there the same way as on any Debian 13 computer
([INSTALLING.md](INSTALLING.md)).

If the 3-D view is slow, that is the Chromebook drawing it without the
graphics processor; small molecules are still fine.
