#ifndef FIFO2__H
#define FIFO2__H
 
// NOTE! size must be 2^N
#define FIFO_CREATE( fifo, size )\
  struct {\
    unsigned char buf[size];\
    unsigned short tail;\
    unsigned short head;\
  }fifo 
 
// current number of elements in queue
#define FIFO_COUNT(fifo)     (fifo.head-fifo.tail)
 
// total number of elements in queue
#define FIFO_SIZE(fifo)      ( sizeof(fifo.buf)/sizeof(fifo.buf[0]) )
 
// is fifo full?
#define FIFO_IS_FULL(fifo)   (FIFO_COUNT(fifo)==FIFO_SIZE(fifo))
 
// is fifo empty?
#define FIFO_IS_EMPTY(fifo)  (fifo.tail==fifo.head)
 
// size of free space in fifo
#define FIFO_SPACE(fifo)     (FIFO_SIZE(fifo)-FIFO_COUNT(fifo))
 
// put element into queue
#define FIFO_PUSH(fifo, byte) \
  {\
    fifo.buf[fifo.head & (FIFO_SIZE(fifo)-1)]=byte;\
    fifo.head++;\
  } 
 
// get first element from queue
#define FIFO_FRONT(fifo) (fifo.buf[fifo.tail & (FIFO_SIZE(fifo)-1)])
 
// decreese counter of elements in queue
#define FIFO_POP(fifo) \
  {\
      fifo.tail++; \
  }
 
// clear fifo
#define FIFO_FLUSH(fifo) \
  {\
    fifo.tail=0;\
    fifo.head=0;\
  } 
 
#endif //FIFO2__H

	