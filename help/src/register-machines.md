# Register Machines

Register Machines is the window where you tell ECCE about the machines it
can run calculations on: the computer you are sitting at, a workstation,
or a cluster reached over ssh. For each machine you set where the codes
are installed, how ECCE connects, what the job script contains and which
queues exist. For the basics of adding a machine, see
[Machines](machines.md). This page covers the window's tabs, how its two
layers of settings work, and examples of common changes.

Open it in the Organizer with **Tools > Register Machines...**, in the
Launcher with **Job > Register Machines...**, or from a terminal with
`ecce -machine`. The machine list is on the left. The tabs on the right
are:

| Tab | What it holds |
|---|---|
| **Machine** | **Machine** (the host name), **Name**, vendor, model, processor, number of processors and nodes. |
| **Connection** | The remote environment (**Shell**, the script run at login), the login host, paths on the remote machine, and **How jobs reach this machine**. |
| **Codes** | One entry per code: **Program** (its path), **Environment variables**, **Command line**, and under **Advanced** commands run before and after the program and files to remove. |
| **Job script** | The request lines for the queue manager, and the commands run before and after the calculation. |
| **Queues** | **Queue manager**, **Allocation accounts used**, and each queue with its limits and defaults. |

The buttons at the bottom are **Help**, **Delete Machine**, **New
Machine**, **Close** and **Save**. A line below the tabs, "Saved in ...",
names the files that **Save** writes for the tab you are looking at.

## Two layers: site setting and user setting

Most settings exist in two layers.

- The **site setting** belongs to the installation. It is shared by
  everyone who uses it and you cannot change it from this window (in
  `ecce -admin` you can; see the last example).
- The **user setting** is your own. It is stored in your home directory
  and applies only to you. **A user setting replaces the site setting; it
  is not added to it.** If the site setting has three lines and you write
  one, the job script gets your one line.

A setting that is a single value, such as a path or the **Shell**, has a
small tag next to it:

| Tag | Meaning |
|---|---|
| **from site** | The value comes from the site. You have not set your own. |
| **your value** | The value is yours. It is stored in your file, and replaces the site's if there is one. |
| **not set** | No layer sets it. ECCE's built-in default applies. |

On a client of a central server the tag reads **from server** instead of
**from site**; see the last example.

A setting that is text, such as the request lines on the **Job script**
tab, shows the site setting in a grey, read-only box (**Site setting**,
with its source below it) and your own in an editable box below it
(**User setting (replaces the site setting)**).

Next to a setting there is a button with an undo arrow when you can go
back:

- Before you save, it puts back the value the setting had when you opened
  the machine or last saved.
- After you save a value of your own over a site setting, it removes your
  value, so the site setting applies again.

**Disable** (a checkbox on the text settings) writes `key: -`, which
means that neither the site setting nor a user setting is used. Use it to
turn off something the site sets, for example a request line that is wrong
for your account. Clear the checkbox to go back.

Changes are not written until you click **Save**; the window title shows
`*` while there are unsaved changes. The footer shows where each tab
saves:

| Tab | Saved in |
|---|---|
| Machine | `~/.ECCE/MyMachines` |
| Connection, Codes, Job script | `~/.ECCE/CONFIG.<name>` |
| Queues | `~/.ECCE/Queues` and `~/.ECCE/<name>.Q` |

`<name>` is the machine's **Name**. Under `ecce -admin` the same files are
in `$ECCE_HOME/siteconfig`, and the Machine tab saves to `Machines`.

Selecting a machine the site supplies shows a note that saving stores
your own copy and that deleting your copy brings the site one back.

## Add a line to the request lines

The site's request lines for your queue manager are usually right except
for one thing, for example a quality-of-service your account needs. You
add a line by copying the site's lines and adding to them.

1. Select the machine and open the **Job script** tab.
2. In the block **Request lines for Slurm** (the name follows the queue
   manager set on the **Queues** tab), click **Copy site setting**. The
   site's lines appear in the **User setting** box.
3. Click at the end of the last line and add a new line:
   `#SBATCH --qos=normal`
4. Click **Save**.

The footer confirms that the file is `~/.ECCE/CONFIG.<name>`. The tag next
to the block now reads **your value**. To return to the site's lines,
click the undo arrow.

## Placeholders

A job script is written once but used for every job, so the request lines
contain *placeholders*: words that start with `$`, which ECCE replaces
when it submits the job. For example,

```
#SBATCH --time=$wallTime
```

becomes, for a two-hour job,

```
#SBATCH --time=02:00:00
```

A request line whose placeholder is empty is left out. Placeholders work
in the request lines, in the commands run before and after the program,
and in a command line. They do not work in environment variables.

To insert one:

1. Click in the text box where it belongs, at the position where it
   should go. This can be a box on the **Job script** tab or the
   **Command line** box on the **Codes** tab.
2. Click the link **Placeholders: $queue, $nodes, ...** above the boxes.
   The window **Placeholders** opens. Its last line, **Insert goes into**,
   names the box that will receive the word.
3. Select a placeholder in the list and click **Insert**, or double-click
   it.
4. Click **Close**, then **Save**.

You can also type a placeholder yourself. Frequently used ones are
`$queue`, `$nodes`, `$totalprocs`, `$ppn`, `$wallTime`, `$memory` and
`$scratchDir`; the window lists all of them with what each becomes.

## Set the environment for Gaussian 16

Gaussian 16 needs `g16root` and a scratch directory before it starts. Set
them for this machine on the **Codes** tab.

1. Select the machine and open the **Codes** tab.
2. In the list, select **Gaussian-16**. Codes with a program path are
   marked with `*` and listed first.
3. In **Program**, enter the path of the program, for example
   `/opt/g16/g16`, if it is not set already.
4. In **Environment variables**, click in the **User setting** box and
   enter one variable per line, as the name, a space and the value:

   ```
   g16root /opt
   GAUSS_SCRDIR /scratch
   ```
5. Click **Save**.

ECCE exports these in the job script before it starts the program. A name
that contains `PATH` is added to the end of what the variable already
holds; any other name replaces it. The settings are stored in
`~/.ECCE/CONFIG.<name>` in the block `Gaussian-16Environment`.

## Queue limits and defaults

The **Queues** tab describes each queue of the machine, so that the
Launcher can offer the right choices and check a request before it is
sent.

1. Open the **Queues** tab.
2. In **Queue manager**, choose the batch system, for example Slurm.
   Tick **Allocation accounts used** if jobs are charged to an account.
3. Choose a queue in **Queues**, or type a new **Name**.
4. For each row, enter the limit in the field marked **max** (and
   **min** for **Processors**), and the value the Launcher should start
   with in the field marked **default**:

   | Row | Unit |
   |---|---|
   | **Processors** | number |
   | **Wall time** | hours (a fraction such as 0.5 is allowed) |
   | **Memory** | GB |
   | **Scratch** | GB |

   A **max** of 0 means no limit. A **default** of 0 means none; the
   Launcher then uses its own.
5. Click **Add Queue**. Repeat from step 3 for each further queue.
   **Remove Queue** deletes the chosen queue and **Remove All** deletes
   all of them.
6. Click **Save**. Queue changes are kept in the list until you do.

The limits are what the Launcher checks a request against, and the upper
bound of its fields (for **Scratch**, the **max** is the most a job may
request). The **default** fills a Launcher field only when you have not
used a value of your own for that machine and queue before; after that
the Launcher remembers what you used.

## Find the queues of a cluster

Instead of typing the queues, ask the machine's queue manager for them.
**Discover queues...** runs the manager's own listing command on the
machine (over ssh for a remote machine) and fills in each queue's limits.
It works for Slurm (`sinfo`), PBS (`qstat -Qf`), Grid Engine (`qconf -sql`
and `qconf -sq`), LSF (`bqueues -l`) and HTCondor (`condor_status`; a pool
has no queues, so it is offered as one queue called `pool`). For other
queue managers, enter the queues by hand.

1. Open the **Queues** tab and choose the **Queue manager**. If the scheduler's
   commands are not on the machine's default path, enter their directory on
   the **Connection** tab, in **Directory of sbatch, squeue, ...**. Discovery
   uses the value in the window now, saved or not.
2. Click **Discover queues...**. ECCE connects to the machine. The window
   **Discover queues** opens and names the command that ran.
3. Tick the queues to add. **Select All** and **Select None** change every row.
4. Click **Add Queues**. Each queue is added to the list, or, if it is in the
   list already, its limits are updated and its defaults are kept.
5. Choose a queue in **Queues** and check its fields. Enter the **default**
   values yourself; discovery leaves them empty.
6. Click **Save**.

The scheduler reports what it knows: **Processors** is the most one job can
use in that queue (for Slurm, all the processors of the partition), **Wall
time** is the time limit in hours (no limit is 0), and **Memory** is the
largest memory of one node, in GB. If the machine cannot be reached, or the
queue manager prints nothing, the window shows the message the machine gave
and adds nothing.

## Codes or Job script?

Both tabs put commands into the job script. The **Job script** tab applies
to every job on the machine, whatever the code. The **Codes** tab applies
to one code. The job script is written in this order:

```
#SBATCH ...              Job script: request lines
module load ...          Job script: commands before the calculation
export g16root=/opt ...  Codes: environment variables
g16 < input > output     Codes: program and command line
cp *.log ...             Job script: commands after the calculation
```

A code's own commands before and after the program (**Codes > Advanced**)
**replace** the machine-wide ones on the **Job script** tab for that code;
they are not added to them. For the other codes the machine-wide commands
still apply.

Put a command on the **Job script** tab if every code needs it, for
example `module load openmpi`. Put it on the **Codes** tab if only one
code does, for example the environment of Gaussian 16.

## Advanced: edit file

On the **Job script** tab, **Advanced: edit file...** opens the machine's
whole settings file (`~/.ECCE/CONFIG.<name>`) as text. Use it when
something is not on the form: a setting that has no field, a comment you
want to keep, or several changes you prefer to make in one place. Its
settings are described in `GETTING_STARTED.md`.

1. Select the machine and open the **Job script** tab.
2. Click **Advanced: edit file...**. If the form has unsaved changes, ECCE
   asks you to save or discard them first, because the form and the file
   are never merged.
3. Edit the text.
4. Click **Check**. It reports unclosed blocks and names keys ECCE does
   not read, so a misspelled setting is found before it is lost.
5. Click **Save**. A file with an error, such as a block that is not
   closed, is not saved. The window then shows the form again with the
   new values.

Save the machine once before using this; a machine that has no settings
file yet has nothing to edit.

## Machines ECCE cannot submit to

Two settings on the **Connection** tab change how a job reaches the
machine: **Submit jobs interactively** (you submit the job script
yourself and give ECCE the job ID) and **Don't submit; only make the
files** (ECCE only writes the files, for example when the machine needs
two-factor authentication). Both are under **Advanced > Job submission**
and are described in
[Job submission settings](machines.md#job-submission-settings).

## On a central server

When ECCE runs against a central server (`ecce -remote`), the site
settings are the server's: the machine list and the CONFIG files the
administrator has registered. The window works the same way, with these
differences.

**What a user can change.** A user cannot change the server's settings.
Selecting a machine from the server shows a note that saving stores your
own copy. Your copies are files in `~/.ECCE` on your own computer, and
they replace the server's values for you only. The tag next to a setting
that comes from the server reads **from server**, and the undo arrow
removes your copy of that value. **Delete Machine** removes your copy of
a machine and brings the server's back.

**How an administrator changes the site settings.** On the server, run

```
sudo ecce -admin
```

This opens Register Machines on the files in `$ECCE_HOME/siteconfig`
without starting a session. It needs write access to that directory,
hence `sudo`. The labels change: the layers are **Default setting** and
**Site setting (replaces the default)**, and **Save** writes to
`siteconfig` and not to `~/.ECCE`. Where a client keeps a copy of the
server's machine list, re-run `sudo ecce-remote-setup <server-host>` on
the client after changing the list.
