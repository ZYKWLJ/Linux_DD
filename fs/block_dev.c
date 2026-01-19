/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+                                                               +
+  本文链接：https://mp.weixin.qq.com/s/Q8B3_YfFZZKnNu9IxGb70g  
+  文件功能：block_dev.c 程序属于块设备文件数据访问操作类程序。
该文件包括block_read()  和block_write()两个块设备读写函数。 
这两个函数是供系统调用函数read() 和 write()调用的， 其他地方没有引用 。  
+                                                               +
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

#include <errno.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/system.h>

int block_write(int dev, long *pos, char *buf, int count)
{
    int block = *pos >> BLOCK_SIZE_BITS;
    int offset = *pos & (BLOCK_SIZE - 1);
    int chars;
    int written = 0;
    struct buffer_head *bh;
    register char *p;

    while (count > 0)
    {
        chars = BLOCK_SIZE - offset;
        if (chars > count)
            chars = count;
        if (chars == BLOCK_SIZE)
            bh = getblk(dev, block);
        else
            bh = breada(dev, block, block + 1, block + 2, -1);
        block++;
        if (!bh)
            return written ? written : -EIO;
        p = offset + bh->b_data;
        offset = 0;
        *pos += chars;
        written += chars;
        count -= chars;
        while (chars-- > 0)
            *(p++) = get_fs_byte(buf++);
        bh->b_dirt = 1;
        brelse(bh);
    }
    return written;
}

int block_read(int dev, unsigned long *pos, char *buf, int count)
{
    int block = *pos >> BLOCK_SIZE_BITS;
    int offset = *pos & (BLOCK_SIZE - 1);
    int chars;
    int read = 0;
    struct buffer_head *bh;
    register char *p;

    while (count > 0)
    {
        chars = BLOCK_SIZE - offset;
        if (chars > count)
            chars = count;
        if (!(bh = breada(dev, block, block + 1, block + 2, -1)))
            return read ? read : -EIO;
        block++;
        p = offset + bh->b_data;
        offset = 0;
        *pos += chars;
        read += chars;
        count -= chars;
        while (chars-- > 0)
            put_fs_byte(*(p++), buf++);
        brelse(bh);
    }
    return read;
}
