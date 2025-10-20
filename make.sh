set -x -e

# Get misra
MISRA_REPO='https://github.com/furdog/MISRA.git'
if cd misra; then git pull; cd ..; else git clone "$MISRA_REPO"; fi
MISRA='./MISRA/misra.sh'
"$MISRA" setup

HEADER="*.h"
SOURCE="*.test.c"

"$MISRA" check ${HEADER}

gcc ${SOURCE} -std=c89 -pedantic -Wall -Wextra -g \
	      -fsanitize=undefined -fsanitize-undefined-trap-on-error

./a
rm a

doxygen doc/Doxyfile
