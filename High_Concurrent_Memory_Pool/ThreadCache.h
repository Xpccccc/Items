#pragma once

#include "Comm.h"

class ThreadCache
{
public:
    void *Allocate(size_t size);

    void DeAllocate(void *ptr, size_t size);

private:
};
