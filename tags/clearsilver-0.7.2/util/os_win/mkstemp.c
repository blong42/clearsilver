
#include <fcntl.h>

int mkstemp(char *path) {
  return open(mktemp(path),O_RDWR);
}
