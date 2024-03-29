#ifndef UTIL_LINUX_PT_BSD_H
#define UTIL_LINUX_PT_BSD_H

#include<stdint.h>

#define BSD_MAXPARTITIONS	16
#define BSD_FS_UNUSED		0

#ifndef BSD_DISKMAGIC
# define BSD_DISKMAGIC     ((uint32_t) 0x82564557)
#endif

#define BSD_LINUX_BOOTDIR "/usr/ucb/mdec"

#if defined (__alpha__) || defined (__powerpc__) || \
    defined (__ia64__) || defined (__hppa__)
# define BSD_LABELSECTOR   0
# define BSD_LABELOFFSET   64
#else
# define BSD_LABELSECTOR   1
# define BSD_LABELOFFSET   0
#endif

#define	BSD_BBSIZE        8192		/* size of boot area, with label */
#define	BSD_SBSIZE        8192		/* max size of fs superblock */

struct bsd_disklabel {
	uint32_t	d_magic;		/* the magic number */
	int16_t		d_type;			/* drive type */
	int16_t		d_subtype;		/* controller/d_type specific */
	char		d_typename[16];		/* type name, e.g. "eagle" */
	char		d_packname[16];		/* pack identifier */

			/* disk geometry: */
	uint32_t	d_secsize;		/* # of bytes per sector */
	uint32_t	d_nsectors;		/* # of data sectors per track */
	uint32_t	d_ntracks;		/* # of tracks per cylinder */
	uint32_t	d_ncylinders;		/* # of data cylinders per unit */
	uint32_t	d_secpercyl;		/* # of data sectors per cylinder */
	uint32_t	d_secperunit;		/* # of data sectors per unit */

	/*
	 * Spares (bad sector replacements) below
	 * are not counted in d_nsectors or d_secpercyl.
	 * Spare sectors are assumed to be physical sectors
	 * which occupy space at the end of each track and/or cylinder.
	 */
	uint16_t	d_sparespertrack;	/* # of spare sectors per track */
	uint16_t	d_sparespercyl;		/* # of spare sectors per cylinder */

	/*
	 * Alternate cylinders include maintenance, replacement,
	 * configuration description areas, etc.
	 */
	uint32_t	d_acylinders;		/* # of alt. cylinders per unit */

			/* hardware characteristics: */
	/*
	 * d_interleave, d_trackskew and d_cylskew describe perturbations
	 * in the media format used to compensate for a slow controller.
	 * Interleave is physical sector interleave, set up by the formatter
	 * or controller when formatting.  When interleaving is in use,
	 * logically adjacent sectors are not physically contiguous,
	 * but instead are separated by some number of sectors.
	 * It is specified as the ratio of physical sectors traversed
	 * per logical sector.  Thus an interleave of 1:1 implies contiguous
	 * layout, while 2:1 implies that logical sector 0 is separated
	 * by one sector from logical sector 1.
	 * d_trackskew is the offset of sector 0 on track N
	 * relative to sector 0 on track N-1 on the same cylinder.
	 * Finally, d_cylskew is the offset of sector 0 on cylinder N
	 * relative to sector 0 on cylinder N-1.
	 */
	uint16_t	d_rpm;			/* rotational speed */
	uint16_t	d_interleave;		/* hardware sector interleave */
	uint16_t	d_trackskew;		/* sector 0 skew, per track */
	uint16_t	d_cylskew;		/* sector 0 skew, per cylinder */
	uint32_t	d_headswitch;		/* head switch time, usec */
	uint32_t	d_trkseek;		/* track-to-track seek, usec */
	uint32_t	d_flags;		/* generic flags */
	uint32_t	d_drivedata[5];		/* drive-type specific information */
	uint32_t	d_spare[5];		/* reserved for future use */
	uint32_t	d_magic2;		/* the magic number (again) */
	uint16_t	d_checksum;		/* xor of data incl. partitions */

			/* filesystem and partition information: */
	uint16_t	d_npartitions;	        /* number of partitions in following */
	uint32_t	d_bbsize;	        /* size of boot area at sn0, bytes */
	uint32_t	d_sbsize;	        /* max size of fs superblock, bytes */

	struct bsd_partition	 {		/* the partition table */
		uint32_t	p_size;	        /* number of sectors in partition */
		uint32_t	p_offset;       /* starting sector */
		uint32_t	p_fsize;        /* filesystem basic fragment size */
		uint8_t		p_fstype;	/* filesystem type, see below */
		uint8_t		p_frag;		/* filesystem fragments per block */
		uint16_t	p_cpg;	        /* filesystem cylinders per group */
	} __attribute__((packed)) d_partitions[BSD_MAXPARTITIONS];	/* actually may be more */
} __attribute__((packed));


/* d_type values: */
#define	BSD_DTYPE_SMD		1		/* SMD, XSMD; VAX hp/up */
#define	BSD_DTYPE_MSCP		2		/* MSCP */
#define	BSD_DTYPE_DEC		3		/* other DEC (rk, rl) */
#define	BSD_DTYPE_SCSI		4		/* SCSI */
#define	BSD_DTYPE_ESDI		5		/* ESDI interface */
#define	BSD_DTYPE_ST506		6		/* ST506 etc. */
#define	BSD_DTYPE_HPIB		7		/* CS/80 on HP-IB */
#define BSD_DTYPE_HPFL		8		/* HP Fiber-link */
#define	BSD_DTYPE_FLOPPY	10		/* floppy */

/* d_subtype values: */
#define BSD_DSTYPE_INDOSPART	0x8		/* is inside dos partition */
#define BSD_DSTYPE_DOSPART(s)	((s) & 3)	/* dos partition number */
#define BSD_DSTYPE_GEOMETRY	0x10		/* drive params in label */

/*
 * Filesystem type and version.
 * Used to interpret other filesystem-specific
 * per-partition information.
 */
#define	BSD_FS_UNUSED	0		/* unused */
#define	BSD_FS_SWAP    	1		/* swap */
#define	BSD_FS_V6      	2		/* Sixth Edition */
#define	BSD_FS_V7      	3		/* Seventh Edition */
#define	BSD_FS_SYSV    	4		/* System V */
#define	BSD_FS_V71K    	5		/* V7 with 1K blocks (4.1, 2.9) */
#define	BSD_FS_V8      	6		/* Eighth Edition, 4K blocks */
#define	BSD_FS_BSDFFS	7		/* 4.2BSD fast file system */
#define	BSD_FS_BSDLFS	9		/* 4.4BSD log-structured file system */
#define	BSD_FS_OTHER	10		/* in use, but unknown/unsupported */
#define	BSD_FS_HPFS	11		/* OS/2 high-performance file system */
#define	BSD_FS_ISO9660	12		/* ISO-9660 filesystem (cdrom) */
#define BSD_FS_ISOFS	BSD_FS_ISO9660
#define	BSD_FS_BOOT	13		/* partition contains bootstrap */
#define BSD_FS_ADOS	14		/* AmigaDOS fast file system */
#define BSD_FS_HFS	15		/* Macintosh HFS */
#define BSD_FS_ADVFS	16		/* Digital Unix AdvFS */

/* this is annoying, but it's also the way it is :-( */
#ifdef __alpha__
#define	BSD_FS_EXT2	8		/* ext2 file system */
#else
#define	BSD_FS_MSDOS	8		/* MS-DOS file system */
#endif

/*
 * flags shared by various drives:
 */
#define	BSD_D_REMOVABLE	0x01		/* removable media */
#define	BSD_D_ECC      	0x02		/* supports ECC */
#define	BSD_D_BADSECT	0x04		/* supports bad sector forw. */
#define	BSD_D_RAMDISK	0x08		/* disk emulator */
#define	BSD_D_CHAIN    	0x10		/* can do back-back transfers */
#define	BSD_D_DOSPART	0x20		/* within MSDOS partition */

#endif /* UTIL_LINUX_PT_BSD_H */
