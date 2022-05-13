
TBD.  How to organize headers.

Option 1: one huge rexdd.h header.

Option 2: several headers, rexdd.h simply includes them all
          (probably in a subdirectory, e.g., rexdd/forest.h, rexdd/apply.h)


What's contained in each source file:

forest.c  : forest settings, and forest functions.

rexdd.c   : preliminary edge functions, probably should
            become macros in rexdd.h

