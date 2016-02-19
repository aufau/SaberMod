#!/usr/bin/perl

# q3scriptindent.pl by fau <faltec@gmail.com>

#          Usage: q3scriptindent.pl [FILE]

#    Description: Changes the appearance of a Quake III: Arena script
#                 files and fixes some common errors. Tested on .menu
#                 and hud files.

use strict;
use warnings;

use constant VALUE_PAD => 24;
use constant COMMENT_PAD => 24;

my $indent = 0;
my $drop_new_lines = 0;
my $open_block = 0;
my @words;
my $line;
my $comment;
my $padding;

while (<>) { # Read line from file or stdin
    s/^\s*//; # Remove leading whitespace
    s/\r\n$/\n/; # Replace DOS line endings

    if (length == 0) {
        next if $drop_new_lines; # Don't print more more than 1 empty line
        print "\n";
        $drop_new_lines = 1;
        next;
    }
    $drop_new_lines = 0;

    ($line, $comment) = split(/\/\//, $_, 2);

    if (length $line > 0) {
        $line =~ s/\;/ \; /; # Make sure that ; and { go into separate words
        $line =~ s/[^^]\{/ \{ /;

        @words = split(/\s+/, $line);
        $padding = VALUE_PAD - length($words[0]);

        if ($words[0] eq "}") {
            $padding = 1;
            if ($indent == 0) { # Remove excessive } braces
                if (defined $comment) {
                    $comment =~s/^\s*//;
                    print "// $comment";
                }
                next;
            } else {
                $indent--;
            }
        }

        print "\t" x $indent;
        print $words[0];
        $line = join(" ", @words[0..$#words]);

        if ($words[0] eq "{") {
            $padding = 1; # For one-line blocks
            $indent++ unless (grep /}/, @words); # Detect one-line blocks
        }

        if ($#words > 0 && $words[$#words] eq "{") { # Move opening { to new line
            pop @words;
            $open_block = 1;
        }

        if ($#words > 0) { # Print line with padding
            $padding = $padding > 1 ? $padding : 1;
            $padding = 1 if ($words[1] eq ";");

            print " " x $padding;
            $line = join(" ", @words[1..$#words]);
            print $line;
        }
        if (defined $comment) { # Print comment with padding
            chomp($comment);
            $comment =~ s/^\s*//;
            $padding = COMMENT_PAD - length($line);
            print " " x ($padding > 1 ? $padding : 1);
            print "// $comment";
        }
    } else { # Line contains only comment
        print "\t" x $indent;
        print "//", join(" ", split(/\s+/, $comment));
    }

    print "\n";

    if ($open_block) { # Move opening { to new line
        print "\t" x $indent;
        print "{\n";
        $indent++;
        $open_block = 0;
    }
}

# Print missing } braces
while($indent > 0) {
    $indent--;
    print "\t" x $indent;
    print "}\n";
}
