MISRA='../../../MISRA/misra.sh'

"$MISRA" check canary_log_reader.h
#"$MISRA" check canary_log_reader.test.c

gcc canary_log_reader.test.c -Wall -Wextra -g -std=c89 -pedantic
./a

rm a
