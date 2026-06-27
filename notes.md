Channel, Window is a split line of our engine. At this stage, our engine is functional to some extent.
It is still at a very early stage tho.

What is the status of Window?

For a Logical level, it is just a spec: WS, WA, func.
At the physical level, which means that this code is ran directly on the metal machine. 

When a tuple comes, the window push it to the vector. then checks if the time is passed the window end, if yes. call func_ on the values, then slide the window. push the result to the downside. 