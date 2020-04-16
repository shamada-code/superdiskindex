

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <sys/mman.h>

#pragma pack(1)

struct bpb_dos30 {
    uint16_t byte_per_sect;
    uint8_t sect_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_entries;
    uint16_t sector_count;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t head_count;
};

struct bootsect_st {
    uint16_t jump_instr;
    uint8_t oem_name[6];
    uint8_t serial[3];
    bpb_dos30 bpb;
};

struct dir_entry {
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attrs;
    uint8_t unk1;
    uint8_t create_time;
    uint16_t create_time2;
    uint16_t create_date;
    uint16_t access_time;
    uint16_t perm;
    uint16_t mod_time;
    uint16_t mod_date;
    uint16_t first_cluster;
    uint32_t size;
};

enum direntry_attr {
    DEA_RO = 0x01,
    DEA_HID = 0x02,
    DEA_SYS = 0x04,
    DEA_VOL = 0x08,
    DEA_DIR = 0x10,
    DEA_ARC = 0x20,
    DEA_DEV = 0x40,
    DEA_RES = 0x80,
};

uint16_t get_fat_entry(uint8_t *fat, uint16_t cluster)
{
    uint16_t pair = cluster>>1;
    if ((cluster%2)==0) 
        return ((fat[3*pair+1]&0x0f)<<8)+(fat[3*pair+0]);
    else 
        return ((fat[3*pair+2])<<4)+((fat[3*pair+1]&0xf0)>>4);
}

void scan_cluster_chain(uint8_t *disk, uint16_t first_cluster)
{
    bootsect_st *bs = (bootsect_st *)disk;
    uint_fast8_t *fat0 = (disk+bs->bpb.byte_per_sect);

    uint16_t cluster = first_cluster;
    while (cluster<0xff0) {
        uint16_t next = get_fat_entry(fat0, cluster);
        printf("[%03x] -> %03x\n", cluster, next);
        cluster=next;
    }
}

uint16_t get_size_on_disk(uint8_t *disk, uint16_t first_cluster)
{
    bootsect_st *bs = (bootsect_st *)disk;
    uint_fast8_t *fat0 = (disk+bs->bpb.byte_per_sect);

    uint16_t cluster_count=0;
    uint16_t cluster = first_cluster;
    while (cluster<0xff0) {
        uint16_t next = get_fat_entry(fat0, cluster);
        cluster_count++;
        cluster=next;
    }
    return cluster_count;
}

uint8_t *load_file(uint8_t *disk, uint16_t first_cluster)
{
    bootsect_st *bs = (bootsect_st *)disk;
    uint_fast8_t *fat0 = (disk+bs->bpb.byte_per_sect);
    uint16_t cs = bs->bpb.sect_per_cluster*bs->bpb.byte_per_sect;
    uint32_t size_in_cl = get_size_on_disk(disk, first_cluster);

    uint8_t *memfile = (uint8_t *)malloc(size_in_cl*cs);

    uint8_t *base = (disk+bs->bpb.byte_per_sect*(bs->bpb.sectors_per_fat*bs->bpb.fat_count+1)+32*bs->bpb.root_entries);
    //printf("DBG>> base=%08x\n", base-disk);
    int n=0;
    uint16_t cluster = first_cluster;
    while (cluster<0xff0) {
        uint8_t *src = base+((cluster-2)*cs);
        uint8_t *dst = memfile+(n*cs);
        //printf("DBG>> copying from %08x to %08x\n", src-disk, dst-memfile);
        memcpy(dst, src, cs);
        cluster = get_fat_entry(fat0, cluster);
        n++;
    }

    return memfile;
}

void close_file(uint8_t *p)
{
    free(p);
}

void scan_dir(uint8_t *disk, uint8_t *start, uint16_t entries, int indent=0)
{
    bootsect_st *bs = (bootsect_st *)disk;
    dir_entry *de = (dir_entry *)start;

    for (int i=0; i<entries; i++)
    {
        if ((de->name[0]!=0xe5)&&(de->name[0]!=0x00)&&(de->name[0]!='.'))
        {
            for (int pad=0; pad<indent; pad++) printf(" ");
            printf("%.8s.%.3s %c%c%c%c%c%c $%03x %d\n", 
                de->name, de->ext, 
                de->attrs&DEA_RO?'R':' ',
                de->attrs&DEA_HID?'H':' ',
                de->attrs&DEA_SYS?'S':' ',
                de->attrs&DEA_VOL?'V':' ',
                de->attrs&DEA_DIR?'D':' ',
                de->attrs&DEA_ARC?'A':' ',
                de->first_cluster, de->size);
            if ( (de->attrs&DEA_DIR) && (de->name[0]!='.') )
            {
                uint16_t cs = get_size_on_disk(disk, de->first_cluster)*bs->bpb.sect_per_cluster*bs->bpb.byte_per_sect;
                uint8_t *subdir = load_file(disk, de->first_cluster);
                scan_dir(disk, subdir, cs/32, indent+2);
                close_file(subdir);
            }
        } else {
            //printf("[%04x] ---\n", 
            //    i);
        }
        de++;
    }
}

void compare_fats(uint8_t *disk)
{
    bootsect_st *bs = (bootsect_st *)disk;
    uint32_t fatsize = bs->bpb.sectors_per_fat*bs->bpb.byte_per_sect;
    uint_fast8_t *fat0 = (disk+bs->bpb.byte_per_sect);
    uint_fast8_t *fat1 = (disk+bs->bpb.byte_per_sect+fatsize);

    int errors=0;
    for (uint32_t i = 0; i < fatsize; i++)
    {
        if (fat0[i]!=fat1[i]) {
            //printf("FAT: mismatch in byte $%x. (%02x!=%02x)\n", i, fat0[i], fat1[i]);
            errors++;
        }
    }
    printf("FAT: check done. There were %d bytes with errors.\n", errors);
}

void print_cluster_map(uint8_t *disk)
{
    bootsect_st *bs = (bootsect_st *)disk;
    uint32_t fatsize = bs->bpb.sectors_per_fat*bs->bpb.byte_per_sect;
    uint_fast8_t *fat0 = (disk+bs->bpb.byte_per_sect+fatsize*0);
    uint_fast8_t *fat1 = (disk+bs->bpb.byte_per_sect+fatsize*1);

    for (uint32_t i=0; i<fatsize/3; i++)
    {
        uint16_t c0a = ((fat0[i*3+1]&0x0f)<<8) + ((fat0[i*3+0]&0xff)<<0);
        uint16_t c1a = ((fat0[i*3+2]&0xff)<<4) + ((fat0[i*3+1]&0xf0)>>4);
        uint16_t c0b = ((fat1[i*3+1]&0x0f)<<8) + ((fat1[i*3+0]&0xff)<<0);
        uint16_t c1b = ((fat1[i*3+2]&0xff)<<4) + ((fat1[i*3+1]&0xf0)>>4);
        printf("%c%c", c0a!=c0b?'!':(c0a==0?'.':'D'), c1a!=c1b?'!':(c1a==0?'.':'D'));
        if (i%60==59) printf("\n");
    }
    printf("\n");
}

void print_disk_params(uint8_t *disk)
{
    bootsect_st *bs = (bootsect_st *)disk;
    printf("BPB:\n");
    printf("Sectors per Track: %d\n", bs->bpb.sectors_per_track);
    printf("Number of Heads: %d\n", bs->bpb.head_count);
    printf("Byte per sector: %d\n", bs->bpb.byte_per_sect);
    printf("Sectors per cluster: %d\n", bs->bpb.sect_per_cluster);
    printf("Media descriptor: %02x\n", bs->bpb.media_descriptor);
    printf("Number of FATs: %d\n", bs->bpb.fat_count);
    printf("Sectors per FAT: %d\n", bs->bpb.sectors_per_fat);
    printf("Reserved Sectors: %d\n", bs->bpb.reserved_sectors);
    printf("\n");
}

int main(int argc, char **argv)
{
    printf("SuperDiskIndex v0.0\n\n");

    if (argc!=2) {
        printf("Syntax: %s <diskimage>\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1], O_RDONLY);
    //printf("DBG> fd=%d\n", fd);
    struct stat sbuf;
    fstat(fd, &sbuf);
    //printf("DBG> fsize=%d\n", sbuf.st_size);
    uint8_t *disk = (uint8_t *)mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    //printf("DBG> disk=%p\n", disk);
    if (disk==NULL)
    {
        printf("Failed to mmap the input file!\n");
        close(fd);
        return -2;
    }

    print_disk_params(disk);

    // find root dir
    bootsect_st *bs = (bootsect_st *)disk;
    uint8_t *rootstart = (disk+bs->bpb.byte_per_sect*(bs->bpb.sectors_per_fat*bs->bpb.fat_count+1));
    printf("DBG>> root@%x\n", rootstart-disk);
    scan_dir(disk, rootstart, bs->bpb.root_entries);

    compare_fats(disk);

    print_cluster_map(disk);

    munmap(disk, sbuf.st_size);
    close(fd);

    return 0;
}