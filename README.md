## Idea

There is one circular buffer indexed by sequence number & (NUM_NODES - 1) and a contiguous piece of memory 
that is reserved on each reserve.

Have a node containing the state of the cell if it is reserved, commited or empty.
For each cell I store the pointer after being reserved.