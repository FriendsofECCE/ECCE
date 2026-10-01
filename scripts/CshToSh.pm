package CshToSh;
#
# csh -> POSIX sh for the shell snippets of ECCE's job-script configuration
# (setup, wrapup, <Code>Command, <Code>_loophole in submit.site and CONFIG.*).
#
# The one place the rules live: gensub uses snippet() in its basic mode (translate
# the plain forms, refuse the rest), ecce-csh2sh in structured mode (which also
# rewrites if/foreach/while and source).  A construct is converted only when its
# sh form means exactly the same; everything else is left and reported.
#
use strict;
use warnings;

# Constructs with no mechanical translation, each with the sh form to write.
# First match wins.  Guessing wrong here runs the wrong job.
our @CSH_ONLY = (
  [qr/^if\s*\(/,            'if [ ... ]; then ... fi'],
  [qr/^else\s+if\b/,        'elif [ ... ]; then'],
  [qr/^(?:endif|endsw|end)\s*$/, 'fi, done or esac'],
  [qr/^foreach\b/,          'for v in ...; do ... done'],
  [qr/^while\s*\(/,         'while [ ... ]; do ... done'],
  [qr/^(?:switch|case|default:|breaksw)\b/, 'case ... in pattern) ... ;; esac'],
  [qr/^repeat\b/,           'a for loop'],
  [qr/^@\s/,                'x=$((expression))'],
  [qr/^set\s+\w+\s*=\s*\(/, 'a string, e.g. x="a b" (PATH="dir:$PATH" for path)'],
  [qr/^set\s+\w+\s*=\s*\S+\s+\S/, 'x="value with spaces"'],
  [qr/^set\s+\w+\s*$/,      'x=""'],
  [qr/^setenv\b/,           'export NAME=value'],
  [qr/^source\s/,           '. file   (the file itself must be sh syntax)'],
  [qr/^(?:limit|unlimit)\b/, 'ulimit'],
  [qr/^(?:alias|unalias|rehash|onintr|goto)\b/, 'the sh equivalent (alias n=\'cmd\', trap)'],
  [qr/\$\??\{?status\b/,    '$?'],
  [qr/\$\{?\?\w/,           '${NAME+set} (or ${NAME:+set} for non-empty)'],
  [qr/\$#?argv\b/,          '$1, $#, "$@"'],
  [qr/\$\w+:[htre](?![\w])/, '${x%/*} for :h, ${x##*/} for :t'],
  [qr/\$\{\w+:[htre]\}/,    '${x%/*} for :h, ${x##*/} for :t'],
  [qr/\$#\w+/,              'sh has no arrays; ${#x} is a string length (use "$#" for argument counts)'],
  [qr/\$\{?\w+\}?\[/,       'sh has no arrays; keep one value per variable or loop over a list'],
);

my $oneWord = qr/"[^"]*"|'[^']*'|`[^`]*`|[^\s"'`]\S*/;

# The advice for a (stripped) statement csh-only syntax, or undef.
sub refusal {
  my ($body) = @_;
  foreach my $rule (@CSH_ONLY) {
    return $rule->[1] if ($body =~ $rule->[0]);
  }
  return undef;
}

# The plain forms: setenv, set N = V, unsetenv, exit (n), csh redirections.
sub translate_line {
  my ($body) = @_;
  if ($body =~ /^setenv\s+(\w+)\s+($oneWord)$/) {
    $body = "export $1=$2";
  } elsif ($body =~ /^setenv\s+(\w+)$/) {
    $body = "export $1=";
  } elsif ($body =~ /^unsetenv\s+(\w+)$/) {
    $body = "unset $1";
  } elsif ($body =~ /^set\s+(\w+)\s*=\s*($oneWord)\s*(;?\s*)$/ && $2 !~ /^\(/) {
    $body = "$1=$2";
  } elsif ($body =~ /^exit\s*\(\s*(\d+)\s*\)$/) {
    $body = "exit $1";
  }
  $body =~ s/>>&\s*(?![0-9]\b)(\S+)/>> $1 2>&1/g;
  $body =~ s/(?<![0-9&])>&!?\s*(?![0-9]\b)(\S+)/> $1 2>&1/g;
  $body =~ s/\|&/2>&1 |/g;
  $body =~ s/>>!/>>/g;
  $body =~ s/>!/>/g;
  return $body;
}

# The sh file a csh init file is shipped beside, by naming convention:
# x.csh -> x.sh, and modules' init/csh -> init/sh.
sub sh_twin {
  my ($path) = @_;
  my ($dir, $base) = $path =~ m{^(.*/)?([^/]*)$};
  $dir = '' unless defined $dir;
  return $dir . "$1.sh" if ($base =~ /^(.+)\.csh$/);
  return $dir . "sh"    if ($base eq 'csh');
  return undef;
}

# "(...)" at the start of $s, parentheses balanced, quotes respected.
# Returns (inner, rest) or ().
sub _balanced {
  my ($s) = @_;
  return () unless ($s =~ /^\s*\(/);
  $s =~ s/^\s*//;
  my ($depth, $i, $n) = (0, 0, length($s));
  while ($i < $n) {
    my $c = substr($s, $i, 1);
    if ($c eq '"' || $c eq "'" || $c eq '`') {
      my $j = index($s, $c, $i + 1);
      return () if ($j < 0);
      $i = $j;
    } elsif ($c eq '\\') {
      $i++;
    } elsif ($c eq '(') {
      $depth++;
    } elsif ($c eq ')') {
      $depth--;
      if ($depth == 0) {
        return (substr($s, 1, $i - 1), substr($s, $i + 1));
      }
    }
    $i++;
  }
  return ();
}

###########################################################################
# Conditions:  csh expression -> sh test commands
###########################################################################

my %FILETEST = (e => '-e', d => '-d', f => '-f', r => '-r', w => '-w',
                x => '-x', s => '-s', l => '-L');
my %NUMOP = ('<' => '-lt', '>' => '-gt', '<=' => '-le', '>=' => '-ge');

sub _tokens {
  my ($s) = @_;
  my @t;
  my $word = qr/(?:"(?:[^"\\]|\\.)*"|'[^']*'|`[^`]*`|[^\s()&|!=<>"'`]+)+/;
  pos($s) = 0;
  while (pos($s) < length($s)) {
    next if ($s =~ /\G\s+/gc);
    if ($s =~ /\G(\|\||&&|==|!=|=~|!~|<=|>=|<|>|\(|\)|!)/gc) {
      push @t, ['op', $1];
    } elsif ($s =~ /\G($word)/gc) {
      push @t, ['w', $1];
    } else {
      return (undef, "cannot read the condition near '" . substr($s, pos($s), 12) . "'");
    }
  }
  return (\@t, undef);
}

# A word as an sh operand, always quoted.  Dies with the reason when its meaning
# in csh is not certain.
sub _operand {
  my ($w, $o) = @_;
  my @parts = $w =~ /("(?:[^"\\]|\\.)*"|'[^']*'|`[^`]*`|[^"'`]+)/g;
  my $out = '';
  foreach my $p (@parts) {
    if ($p =~ /^"/ || $p =~ /^'/) {
      die "csh history or status syntax inside quotes\n" if ($p =~ /^"/ && $p =~ /\\!|\$\??\{?status\b/);
      $out .= $p;
    } elsif ($p =~ /^`(.*)`$/s) {
      $out .= '"$(' . $1 . ')"';
    } elsif ($p eq '$status' || $p eq '${status}') {
      die "\$status here is not certain to mean the previous command\n" unless ($o->{status_ok});
      $out .= '"$?"';
    } elsif ($p =~ /^[\w\$\/.{}+:,@%^-]+$/) {
      die "a modifier or array form in '$p'\n" if ($p =~ /\$\{?\w+\}?(?::[a-z]|\[)/ || $p =~ /\$#/);
      $out .= ($p =~ /\$/) ? "\"$p\"" : $p;
    } else {
      die "the word '$p' has a pattern, tilde or special character whose csh meaning is not certain\n";
    }
  }
  return $out;
}

sub _numeric_operand {
  my ($w, $o) = @_;
  return $w if ($w =~ /^\d+$/);
  return "\"$w\"" if ($w =~ /^\$\w+$/ || $w =~ /^\$\{\w+\}$/);
  return $1 if ($w =~ /^("\$\w+"|"\$\{\w+\}")$/);
  die "'$w' is not a number or a plain variable\n";
}

sub _parse_cond {
  my ($toks, $o) = @_;
  my $pos = 0;
  my ($or, $and, $not, $primary);

  $primary = sub {
    my $t = $toks->[$pos] or die "the condition ends too soon\n";
    if ($t->[0] eq 'op' && $t->[1] eq '(') {
      $pos++;
      my $n = $or->();
      my $c = $toks->[$pos++];
      die "missing )\n" unless ($c && $c->[0] eq 'op' && $c->[1] eq ')');
      return $n;
    }
    die "unexpected '$t->[1]'\n" if ($t->[0] eq 'op');
    my $w = $t->[1];
    my $next = $toks->[$pos + 1];
    # file inquiry
    if ($w =~ /^-([a-z])$/ && $next && $next->[0] eq 'w') {
      my $k = $1;
      die "file test -$k has no certain sh equivalent here\n" unless exists $FILETEST{$k};
      $pos += 2;
      my $op = _operand($next->[1], $o);
      die "file test on a pattern\n" if ($next->[1] =~ /[*?\[]/);
      return ['atom', "[ $FILETEST{$k} $op ]"];
    }
    # comparison
    if ($next && $next->[0] eq 'op' && $next->[1] =~ /^(?:==|!=|<=|>=|<|>)$/) {
      my $op = $next->[1];
      my $rt = $toks->[$pos + 2];
      die "a comparison without a right-hand side\n" unless ($rt && $rt->[0] eq 'w');
      $pos += 3;
      if ($op eq '==' || $op eq '!=') {
        die "a pattern on the right of $op\n" if ($rt->[1] =~ /(?<![\\])[*?\[]/ && $rt->[1] !~ /^["'`]/);
        my $l = _operand($w, $o);
        my $r = _operand($rt->[1], $o);
        return ['atom', "[ $l " . ($op eq '==' ? '=' : '!=') . " $r ]"];
      }
      return ['atom', "[ " . _numeric_operand($w, $o) . " $NUMOP{$op} " . _numeric_operand($rt->[1], $o) . " ]"];
    }
    die "pattern match ($next->[1]) has no certain sh equivalent\n"
      if ($next && $next->[0] eq 'op' && $next->[1] =~ /^[=!]~$/);
    $pos++;
    if ($w =~ /^\$\?\{?(\w+)\}?$/) {
      return ['atom', "[ -n \"\${$1+set}\" ]"];
    }
    if ($w =~ /^\d+$/) {
      return ['atom', $w == 0 ? 'false' : 'true'];
    }
    if ($w eq '$status') {
      die "\$status here is not certain to mean the previous command\n" unless ($o->{status_ok});
      return ['atom', '[ "$?" -ne 0 ]'];
    }
    die "'$w' alone is not a condition that means the same in sh\n";
  };
  $not = sub {
    my $t = $toks->[$pos];
    if ($t && $t->[0] eq 'op' && $t->[1] eq '!') {
      $pos++;
      return ['not', $not->()];
    }
    return $primary->();
  };
  $and = sub {
    my @n = ($not->());
    while ($toks->[$pos] && $toks->[$pos][0] eq 'op' && $toks->[$pos][1] eq '&&') {
      $pos++;
      push @n, $not->();
    }
    return @n == 1 ? $n[0] : ['and', @n];
  };
  $or = sub {
    my @n = ($and->());
    while ($toks->[$pos] && $toks->[$pos][0] eq 'op' && $toks->[$pos][1] eq '||') {
      $pos++;
      push @n, $and->();
    }
    return @n == 1 ? $n[0] : ['or', @n];
  };
  my $tree = $or->();
  die "unexpected '$toks->[$pos][1]'\n" if ($pos < @$toks);
  return $tree;
}

# sh's && and || have equal precedence, csh's do not: mixed levels get braces.
sub _emit {
  my ($n) = @_;
  my ($k, @c) = @$n;
  return $c[0] if ($k eq 'atom');
  if ($k eq 'not') {
    my $inner = $c[0];
    return '! ' . ($inner->[0] eq 'atom' ? $inner->[1] : '{ ' . _emit($inner) . '; }')
      if ($inner->[0] ne 'not');
    return '! { ' . _emit($inner) . '; }';
  }
  my $other = $k eq 'and' ? 'or' : 'and';
  my $join = $k eq 'and' ? ' && ' : ' || ';
  return join($join, map { $_->[0] eq $other ? '{ ' . _emit($_) . '; }' : _emit($_) } @c);
}

# csh expression text -> (sh command, undef) or (undef, reason).
sub convert_condition {
  my ($expr, %o) = @_;
  my ($toks, $err) = _tokens($expr);
  return (undef, $err) if (!$toks);
  return (undef, "an empty condition") if (!@$toks);
  my $tree = eval { _parse_cond($toks, \%o) };
  if ($@) { my $m = $@; $m =~ s{\n\z}{}; return (undef, $m); }
  return (_emit($tree), undef);
}

###########################################################################
# Snippets
###########################################################################

# $text: the snippet.  Options: structured (also convert if/foreach/while,
# $status and source), assume_twins, exists (code ref, default -e).
# Returns { text, changes => [{line, from, to}], issues => [{line, text, want, why}] }
# with one output line per input line, so line numbers survive.
sub snippet {
  my ($text, %o) = @_;
  my $res = { text => $text, changes => [], issues => [] };
  return $res if (!defined($text) || $text eq "");

  my @lines = split(/\n/, $text, -1);
  my (@new, @skip, %why, @stack, @issues, @changes);
  my $heredoc;
  my $afterOpener = 0;
  my $exists = $o{exists} || sub { -e $_[0] };

  my $bad = sub {                      # drop pending edits of the innermost block
    my ($e) = @_;
    $e->{ok} = 0;
    $e->{edits} = [];
  };

  my $simple = sub {                   # one statement -> (new body, why)
    my ($body, $first) = @_;
    my $why;
    my $new = $body;
    if ($o{structured}) {
      if ($body =~ /^source\s+(\S+)\s*(#.*)?$/) {
        my ($path, $cmt) = ($1, defined($2) ? " $2" : "");
        my $twin = ($path =~ /^[\w\$\/.{}~+:,@%-]+$/) ? sh_twin($path) : undef;
        if (!defined $twin) {
          return ($body, "no sh twin is known for this file name; write the sh version of its contents");
        }
        my $lit = ($path !~ /[\$`]/);
        if ($o{assume_twins} || ($lit && $exists->(($twin =~ /^~/) ? ($ENV{HOME} || '') . substr($twin, 1) : $twin))) {
          if ($twin !~ m{/}) {
            return ($body, "give the sh twin with a path (. ./$twin)") if ($twin =~ /\$/);
            $twin = "./$twin";
          }
          return (". $twin$cmt", undef);
        }
        return ($body, $lit
          ? "$twin not found on this machine; if it exists where the job runs, use: . $twin (or run ecce-csh2sh with --assume-sh-twins)"
          : "the path has a variable, so $twin cannot be checked; if it exists where the job runs, use: . $twin (or --assume-sh-twins)");
      }
      if ($body =~ /\$\{?status\b/) {
        if ($first || $body =~ /'/) {
          return ($body, "\$status is only converted in a command that follows another, never as the first line of a block");
        }
        (my $b = $body) =~ s/\$\{?status\}?(?![\w])/\$?/g;
        $body = $b;
      }
    }
    $new = translate_line($body);
    return ($new, undef);
  };

  my $open = sub {                     # push a block entry, remembering its edits
    my ($kind, $ok) = @_;
    my $e = { kind => $kind, ok => $ok, edits => [] };
    push @stack, $e;
    return $e;
  };

  LINE:
  for (my $i = 0; $i < @lines; $i++) {
    my $line = $lines[$i];
    $new[$i] = $line;
    if (defined $heredoc) {
      $skip[$i] = 1;
      undef $heredoc if ($line =~ /^\s*\Q$heredoc\E\s*$/);
      next;
    }
    if ($line =~ /^\s*(?:#|$)/) { $skip[$i] = 1; next; }

    my ($ind, $body) = ($line =~ /^(\s*)(.*?)\s*$/);
    my $orig = $body;
    my $first = $afterOpener;
    $afterOpener = 0;
    if ($body =~ /<<-?\s*['"]?(\w+)['"]?/) { $heredoc = $1; }

    my $set = sub {                    # record the replacement for line $i
      my ($b) = @_;
      $new[$i] = $ind . $b;
    };

    if ($o{structured}) {
      my $top = $stack[-1];

      # A block statement continued over lines is not rewritten, only reported.
      if ($body =~ /\\$/ && $body =~ /^(if|else\s+if|while|foreach)\b/) {
        my $kw = $1;
        $why{$i} = "continued over several lines; ecce-csh2sh does not rewrite those";
        if ($kw eq 'else if') { $bad->($top) if $top; }
        elsif ($kw eq 'if') { $open->('if', 0) if ($body =~ /\bthen\b/); }
        else { $open->($kw, 0); }
        $afterOpener = 1;
        next;
      }

      # if / else if / single-line if / while
      if ($body =~ /^(if|else\s+if|while)\s*\(/) {
        my $kw = $1;
        $kw =~ s/\s+/ /;
        my ($inner, $rest) = _balanced($body =~ s/^(?:if|else\s+if|while)\s*//r);
        if (!defined $inner) {
          $why{$i} = "unbalanced parentheses";
          if ($kw eq 'if' || $kw eq 'while') { $open->($kw eq 'while' ? 'while' : 'if', 0); }
          elsif ($top) { $bad->($top); }
          $afterOpener = 1;
          next;
        }
        $rest =~ s/^\s+//;
        my $isBlock = ($rest =~ /^then\s*(#.*)?$/);
        my $cmt = ($isBlock && defined $1) ? " $1" : "";
        if ($kw eq 'while') {
          my $e = $open->('while', 1);
          my ($c, $r) = convert_condition($inner, status_ok => 0);
          if (!defined $c || $rest ne '' && $rest !~ /^#/) {
            $why{$i} = defined $r ? $r : "text after the condition";
            $bad->($e);
          } else {
            push @{$e->{edits}}, [$i, $ind . "while $c; do" . ($rest =~ /^#/ ? " $rest" : "")];
          }
          $afterOpener = 1;
          next;
        }
        if ($kw eq 'else if') {
          if (!$top || $top->{kind} ne 'if') { $why{$i} = "else if outside an if block"; $afterOpener = 1; next; }
          if (!$isBlock) { $why{$i} = "else if without then"; $bad->($top); $afterOpener = 1; next; }
          my ($c, $r) = convert_condition($inner, status_ok => 0);
          if (!defined $c) { $why{$i} = $r; $bad->($top); }
          elsif ($top->{ok}) { push @{$top->{edits}}, [$i, $ind . "elif $c; then$cmt"]; }
          $afterOpener = 1;
          next;
        }
        # kw eq 'if'
        if ($isBlock) {
          my $e = $open->('if', 1);
          my ($c, $r) = convert_condition($inner, status_ok => !$first);
          if (!defined $c) { $why{$i} = $r; $bad->($e); }
          else { push @{$e->{edits}}, [$i, $ind . "if $c; then$cmt"]; }
          $afterOpener = 1;
          next;
        }
        # if (expr) command
        if ($rest eq '' || $rest =~ /^[;#]/ || $rest =~ /^then\b/ || $rest =~ /[;|]|&&/) {
          $why{$i} = "the command after the condition is not a single simple command";
          next;
        }
        my ($c, $r) = convert_condition($inner, status_ok => !$first);
        if (!defined $c) { $why{$i} = $r; next; }
        my ($cmd, $cwhy) = $simple->($rest, 1);
        if ($cwhy || defined refusal($cmd)) { $why{$i} = $cwhy || "the command is csh syntax"; next; }
        $set->("if $c; then $cmd; fi");
        next;
      }

      if ($body =~ /^else\s*(#.*)?$/) {
        $afterOpener = 1;
        if (!$top || $top->{kind} ne 'if') { $why{$i} = "else outside an if block"; }
        next;
      }

      if ($body =~ /^endif\s*(#.*)?$/) {
        my $cmt = defined $1 ? " $1" : "";
        if ($top && $top->{kind} eq 'if') {
          pop @stack;
          if ($top->{ok}) {
            $new[$_->[0]] = $_->[1] for @{$top->{edits}};
            $set->("fi$cmt");
          }
        } else {
          $why{$i} = "endif without a matching if";
        }
        next;
      }

      if ($body =~ /^foreach\s+(\w+)\s*(\(.*)$/) {
        my $var = $1;
        my ($inner, $rest) = _balanced($2);
        my $e = $open->('foreach', 1);
        $afterOpener = 1;
        if (!defined $inner || $rest !~ /^\s*(#.*)?$/) {
          $why{$i} = "cannot read the word list"; $bad->($e); next;
        }
        my $cmt = defined $1 ? " $1" : "";
        my ($toks, $err) = _tokens("x " . $inner);   # words only
        my @words;
        my $fail = defined $err ? $err : undef;
        if (!$fail) {
          shift @$toks;
          foreach my $t (@$toks) {
            if ($t->[0] ne 'w') { $fail = "the word list contains '$t->[1]'"; last; }
            my $w = $t->[1];
            if ($w =~ /\$#|\$\?|\$\{?\w+\}?(?::[a-z]|\[)|\$status|\$argv/) { $fail = "'$w' uses a csh-only form"; last; }
            if ($w =~ /^`(.*)`$/s) { push @words, $w; next; }
            if ($w =~ /^["']/ ) { push @words, $w; next; }
            if ($w =~ /[*?\[]/) { push @words, $w; next; }
            if ($w =~ /\$/) { push @words, "\"$w\""; next; }
            push @words, $w;
          }
          $fail = "an empty word list" if (!$fail && !@words);
        }
        if ($fail) { $why{$i} = $fail; $bad->($e); next; }
        push @{$e->{edits}}, [$i, $ind . "for $var in " . join(' ', @words) . "; do$cmt"];
        next;
      }

      if ($body =~ /^end\s*(#.*)?$/) {
        my $cmt = defined $1 ? " $1" : "";
        if ($top && ($top->{kind} eq 'foreach' || $top->{kind} eq 'while')) {
          pop @stack;
          if ($top->{ok}) {
            $new[$_->[0]] = $_->[1] for @{$top->{edits}};
            $set->("done$cmt");
          }
        } else {
          $why{$i} = "end without a matching foreach or while";
        }
        next;
      }
    }

    my ($nb, $why) = $simple->($body, $first);
    $why{$i} = $why if (defined $why);
    if ($nb ne $body) {
      $set->($nb);
    }
  }

  # Edits of blocks never closed stay unapplied; their lines are reported below.
  for (my $i = 0; $i < @lines; $i++) {
    next if ($skip[$i]);
    my ($ind, $body) = ($new[$i] =~ /^(\s*)(.*?)\s*$/);
    my $want = refusal($body);
    next unless (defined $want);
    push @issues, { line => $i + 1, text => $lines[$i] =~ s/^\s+|\s+$//gr, want => $want,
                    why => $why{$i} };
  }
  for (my $i = 0; $i < @lines; $i++) {
    next if ($new[$i] eq $lines[$i]);
    push @changes, { line => $i + 1,
                     from => $lines[$i] =~ s/^\s+|\s+$//gr,
                     to => $new[$i] =~ s/^\s+|\s+$//gr };
  }
  @issues = sort { $a->{line} <=> $b->{line} } @issues;
  return { text => join("\n", @new), changes => \@changes, issues => \@issues };
}

1;
