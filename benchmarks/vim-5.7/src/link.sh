#! /bin/sh
#
# link.sh -- try linking Vim with different sets of libraries, finding the
# minimal set for fastest startup.  The problem is that configure adds a few
# libraries when they exist, but this doesn't mean they are needed for Vim.
#
# Warning: This fails miserably if the linker doesn't return an error code!
#
# Otherwise this script is fail-safe, falling back to the original full link
# command if anything fails.
#
# Author: Bram Moolenaar
#   Date: 1999 June 7

echo "$LINK" >link.cmd
exit_value=0

#
# If link.sed already exists, use it.  We assume a previous run of link.sh has
# found the correct set of libraries.
#
if test -f link.sed; then
  echo "link.sh: The file 'link.sed' exists, which is going to be used now."
  echo "link.sh: If linking fails, try deleting the link.sed file."
  echo "link.sh: If this fails too, try creating an empty link.sed file."
else

#
# If linking works with the full link command, try removing some libraries,
# that are know not to be needed on at least one system.
# Remove pathdef.c if there is a new link command and compile it again.
#
# Note: Can't remove Xext; It links fine but will give an error when running
# gvim with Motif.
#
  cat link.cmd
  if sh link.cmd; then
    touch link.sed
    for libname in SM ICE nsl dnet dnet_stub inet socket dir elf Xmu Xpm x pthread thread readline; do
      if grep "l$libname " link.cmd >/dev/null; then
        if test ! -f link1.sed; then
          echo "link.sh: OK, linking works, let's try removing a few libraries..."
        fi
        echo "link.sh: Trying to remove the $libname library..."
        echo "s/-l$libname  *//g" >link1.sed
        sed -f link.sed <link.cmd >linkit2.sh
        sed -f link1.sed <linkit2.sh >linkit.sh
        cat linkit.sh
        if sh linkit.sh; then
          echo "link.sh: We don't need the $libname library!"
          cat link1.sed >>link.sed
          rm -f pathdef.c
        else
          echo "link.sh: We DO need the $libname library."
        fi
      fi
    done
    if test ! -f pathdef.c; then
      $MAKE pathdef.o
    fi
    if test ! -f link1.sed; then
      echo "link.sh: Linked fine, no libraries can be removed"
      touch link3.sed
    fi
  else
    exit_value=$?
  fi
fi

#
# Now do the real linking.
#
if test -s link.sed; then
  echo "link.sh: Using link.sed file to remove a few libraries"
  sed -f link.sed <link.cmd >linkit.sh
  cat linkit.sh
  if sh linkit.sh; then
    exit_value=0
    echo "link.sh: Linked fine with a few libraries removed"
  else
    exit_value=$?
    echo "link.sh: Linking failed, making link.sed empty and trying again"
    mv -f link.sed link2.sed
    touch link.sed
    rm -f pathdef.c
    $MAKE pathdef.o
  fi
fi
if test -f link.sed -a ! -s link.sed -a ! -f link3.sed; then
  echo "link.sh: Using unmodified link command"
  cat link.cmd
  if sh link.cmd; then
    exit_value=0
    echo "link.sh: Linked OK"
  else
    exit_value=$?
    if test -f link2.sed; then
      echo "link.sh: Linking doesn't work at all, removing link.sed"
      rm -f link.sed
    fi
  fi
fi

#
# cleanup
#
rm -f link.cmd linkit.sh link1.sed link2.sed link3.sed linkit2.sh

#
# return an error code if something went wrong
#
exit $exit_value

# vim:set sw=2 et:
