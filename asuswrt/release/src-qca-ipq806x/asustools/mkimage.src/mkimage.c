/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
/*
 * (C) Copyright 2000-2004
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __WIN32__
#include <netinet/in.h>		/* for host / network byte order conversions	*/
#endif
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#if defined(__BEOS__) || defined(__NetBSD__) || defined(__APPLE__)
#include <inttypes.h>
#endif

#ifdef __WIN32__
typedef unsigned int __u32;

#define SWAP_LONG(x) \
	((__u32)( \
		(((__u32)(x) & (__u32)0x000000ffUL) << 24) | \
		(((__u32)(x) & (__u32)0x0000ff00UL) <<  8) | \
		(((__u32)(x) & (__u32)0x00ff0000UL) >>  8) | \
		(((__u32)(x) & (__u32)0xff000000UL) >> 24) ))
typedef		unsigned char	uint8_t;
typedef		unsigned short	uint16_t;
typedef		unsigned int	uint32_t;

#define     ntohl(a)	SWAP_LONG(a)
#define     htonl(a)	SWAP_LONG(a)
#endif	/* __WIN32__ */

#ifndef	O_BINARY		/* should be define'd on __WIN32__ */
#define O_BINARY	0
#endif

#include <image.h>

extern int errno;

#ifndef MAP_FAILED
#define MAP_FAILED (-1)
#endif

char *cmdname;

extern unsigned long crc32 (unsigned long crc, const char *buf, unsigned int len);

typedef struct table_entry {
	int	val;		/* as defined in image.h	*/
	char	*sname;		/* short (input) name		*/
	char	*lname;		/* long (output) name		*/
} table_entry_t;

table_entry_t arch_name[] = {
	{ IH_ARCH_INVALID,	NULL,		"Invalid CPU",	},
	{ IH_ARCH_ALPHA,	"alpha",	"Alpha",	},
	{ IH_ARCH_ARM,		"arm",		"ARM",		},
	{ IH_ARCH_I386,		"x86",		"Intel x86",	},
	{ IH_ARCH_IA64,		"ia64",		"IA64",		},
	{ IH_ARCH_M68K,		"m68k",		"MC68000",	},
	{ IH_ARCH_MICROBLAZE,	"microblaze",	"MicroBlaze",	},
	{ IH_ARCH_MIPS,		"mips",		"MIPS",		},
	{ IH_ARCH_MIPS64,	"mips64",	"MIPS 64 Bit",	},
	{ IH_ARCH_NIOS,		"nios",		"NIOS",		},
	{ IH_ARCH_NIOS2,	"nios2",	"NIOS II",	},
	{ IH_ARCH_PPC,		"ppc",		"PowerPC",	},
	{ IH_ARCH_S390,		"s390",		"IBM S390",	},
	{ IH_ARCH_SH,		"sh",		"SuperH",	},
	{ IH_ARCH_SPARC,	"sparc",	"SPARC",	},
	{ IH_ARCH_SPARC64,	"sparc64",	"SPARC 64 Bit",	},
	{ IH_ARCH_BLACKFIN,	"blackfin",	"Blackfin",	},
	{ IH_ARCH_AVR32,	"avr32",	"AVR32",	},
	{ IH_ARCH_NDS32,	"nds32",	"NDS32",	},
	{ IH_ARCH_OPENRISC,	"or1k",		"OpenRISC 1000",},
	{ IH_ARCH_ARM64,	"arm64",	"AArch64",	},
	{ -1,			"",		"",		},
};

table_entry_t os_name[] = {
    {	IH_OS_INVALID,	NULL,		"Invalid OS",		},
    {	IH_OS_4_4BSD,	"4_4bsd",	"4_4BSD",		},
    {	IH_OS_ARTOS,	"artos",	"ARTOS",		},
    {	IH_OS_DELL,	"dell",		"Dell",			},
    {	IH_OS_ESIX,	"esix",		"Esix",			},
    {	IH_OS_FREEBSD,	"freebsd",	"FreeBSD",		},
    {	IH_OS_IRIX,	"irix",		"Irix",			},
    {	IH_OS_LINUX,	"linux",	"Linux",		},
    {	IH_OS_LYNXOS,	"lynxos",	"LynxOS",		},
    {	IH_OS_NCR,	"ncr",		"NCR",			},
    {	IH_OS_NETBSD,	"netbsd",	"NetBSD",		},
    {	IH_OS_OPENBSD,	"openbsd",	"OpenBSD",		},
    {	IH_OS_PSOS,	"psos",		"pSOS",			},
    {	IH_OS_QNX,	"qnx",		"QNX",			},
    {	IH_OS_RTEMS,	"rtems",	"RTEMS",		},
    {	IH_OS_SCO,	"sco",		"SCO",			},
    {	IH_OS_SOLARIS,	"solaris",	"Solaris",		},
    {	IH_OS_SVR4,	"svr4",		"SVR4",			},
    {	IH_OS_U_BOOT,	"u-boot",	"U-Boot",		},
    {	IH_OS_VXWORKS,	"vxworks",	"VxWorks",		},
    {	-1,		"",		"",			},
};

table_entry_t type_name[] = {
    {	IH_TYPE_INVALID,    NULL,	  "Invalid Image",	},
    {	IH_TYPE_FILESYSTEM, "filesystem", "Filesystem Image",	},
    {	IH_TYPE_FIRMWARE,   "firmware",	  "Firmware",		},
    {	IH_TYPE_KERNEL,	    "kernel",	  "Kernel Image",	},
    {	IH_TYPE_MULTI,	    "multi",	  "Multi-File Image",	},
    {	IH_TYPE_RAMDISK,    "ramdisk",	  "RAMDisk Image",	},
    {	IH_TYPE_SCRIPT,     "script",	  "Script",		},
    {	IH_TYPE_STANDALONE, "standalone", "Standalone Program", },
    {	-1,		    "",		  "",			},
};

table_entry_t comp_name[] = {
    {	IH_COMP_NONE,	"none",		"uncompressed",		},
    {	IH_COMP_BZIP2,	"bzip2",	"bzip2 compressed",	},
    {	IH_COMP_GZIP,	"gzip",		"gzip compressed",	},
    {	IH_COMP_LZMA,	"lzma",		"lzma compressed",	},
    {	-1,		"",		"",			},
};

static	void	copy_file (int, const char *, int);
static	void	usage	(void);
static	void	print_header (image_header_t *);
static	void	print_type (image_header_t *);
static	char	*put_table_entry (table_entry_t *, char *, int);
static	char	*put_arch (int);
static	char	*put_type (int);
static	char	*put_os   (int);
static	char	*put_comp (int);
static	int	get_table_entry (table_entry_t *, char *, char *);
static	int	get_arch(char *);
static	int	get_comp(char *);
static	int	get_os  (char *);
static	int	get_type(char *);


char	*datafile;
char	*imagefile;

int dflag    = 0;
int eflag    = 0;
int lflag    = 0;
int vflag    = 0;
int xflag    = 0;
int opt_os   = IH_OS_LINUX;
int opt_arch = IH_ARCH_PPC;
int opt_type = IH_TYPE_KERNEL;
int opt_comp = IH_COMP_GZIP;
int vargv    = 0; //added by Chen-I

image_header_t header;
image_header_t *hdr = &header;
TAIL tail_pre;

int
main (int argc, char **argv)
{
	int ifd;
	int i;
	uint32_t checksum;
	uint32_t addr;
	uint32_t ep;
	uint32_t rfs_offset=0;
	struct stat sbuf;
	unsigned char *ptr;
	char *name = "";
#ifdef TRX_NEW
	char *sn = NULL, *en = NULL;
	char tmp[10];
	uint8_t lrand = 0;
	uint8_t rrand = 0;
	uint32_t linux_offset = 0;
	uint32_t rootfs_offset = 0;
#endif	
	memset(&tail_pre, 0, sizeof(tail_pre));

	cmdname = *argv;

	addr = ep = 0;

	while (--argc > 0 && **++argv == '-') {
		while (*++*argv) {
			switch (**argv) {
			case 'l':
				lflag = 1;
				break;
			case 'A':
				if ((--argc <= 0) ||
				    (opt_arch = get_arch(*++argv)) < 0)
					usage ();
				goto NXTARG;
			case 'C':
				if ((--argc <= 0) ||
				    (opt_comp = get_comp(*++argv)) < 0)
					usage ();
				goto NXTARG;
			case 'O':
				if ((--argc <= 0) ||
				    (opt_os = get_os(*++argv)) < 0)
					usage ();
				goto NXTARG;
			case 'T':
				if ((--argc <= 0) ||
				    (opt_type = get_type(*++argv)) < 0)
					usage ();
				goto NXTARG;

			case 'a':
				if (--argc <= 0)
					usage ();
				addr = strtoul (*++argv, (char **)&ptr, 16);
				if (*ptr) {
					fprintf (stderr,
						"%s: invalid load address %s\n",
						cmdname, *argv);
					exit (EXIT_FAILURE);
				}
				goto NXTARG;
			case 'd':
				if (--argc <= 0)
					usage ();
				datafile = *++argv;
				dflag = 1;
				goto NXTARG;
			case 'e':
				if (--argc <= 0)
					usage ();
				ep = strtoul (*++argv, (char **)&ptr, 16);
				if (*ptr) {
					fprintf (stderr,
						"%s: invalid entry point %s\n",
						cmdname, *argv);
					exit (EXIT_FAILURE);
				}
				eflag = 1;
				goto NXTARG;
			case 'r':
				if (--argc <= 0)
					usage ();
				rfs_offset = (uint32_t)strtoul(*++argv, NULL, 0);
				goto NXTARG;
			case 'n':
				if (--argc <= 0)
					usage ();
				name = *++argv;
				goto NXTARG;
			case 'V':
				if ((argc-=10) < 0)
					usage ();
				sscanf(argv[1], "%d.%d", &tail_pre.kernel.major, &tail_pre.kernel.minor);
				sscanf(argv[2], "%d.%d", &tail_pre.fs.major, &tail_pre.fs.minor); 
#ifdef TRX_NEW	
				sscanf(argv[3], "%d", &sn);
				sscanf(argv[4], "%d-%s", &en, tmp);   
				tail_pre.sn = (uint16_t)sn;
				tail_pre.en = (uint16_t)en;
			
				for(i=0; i<3; i++) {
					sscanf(argv[i+5], "%d.%d", &tail_pre.hw[i].major, &tail_pre.hw[i].minor);
				}
#else
				for(i=0; i<(MAX_VER*2); i++) 
					sscanf(argv[i+3], "%d.%d", &tail_pre.hw[i].major, &tail_pre.hw[i].minor);
			
#endif				
				argv+=10;
				vargv=1;
				goto NXTARG;
			case 'v':
				vflag++;
				break;
			case 'x':
				xflag++;
				break;
			default:
				usage ();
			}
		}
NXTARG:		;
	}

	if ((argc != 1) || ((lflag ^ dflag) == 0))
		usage();

	if (!eflag) {
		ep = addr;
		/* If XIP, entry point must be after the U-Boot header */
		if (xflag)
			ep += sizeof(image_header_t);
	}

	/*
	 * If XIP, ensure the entry point is equal to the load address plus
	 * the size of the U-Boot header.
	 */
	if (xflag) {
		if (ep != addr + sizeof(image_header_t)) {
			fprintf (stderr, "%s: For XIP, the entry point must be the load addr + %lu\n",
				cmdname,
				(unsigned long)sizeof(image_header_t));
			exit (EXIT_FAILURE);
		}
	}

	imagefile = *argv;
	fprintf(stderr, "img file: %s\n", imagefile);

	if (lflag) {
		ifd = open(imagefile, O_RDONLY|O_BINARY);
	} else {
		ifd = open(imagefile, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, 0666);
	}

	if (ifd < 0) {
		fprintf (stderr, "%s: Can't open %s: %s\n",
			cmdname, imagefile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	if (lflag) {
		int len;
		char *data;
		/*
		 * list header information of existing image
		 */
		if (fstat(ifd, &sbuf) < 0) {
			fprintf (stderr, "%s: Can't stat %s: %s\n",
				cmdname, imagefile, strerror(errno));
			exit (EXIT_FAILURE);
		}

		if ((unsigned)sbuf.st_size < sizeof(image_header_t)) {
			fprintf (stderr,
				"%s: Bad size: \"%s\" is no valid image\n",
				cmdname, imagefile);
			exit (EXIT_FAILURE);
		}

		ptr = (unsigned char *)mmap(0, sbuf.st_size,
					    PROT_READ, MAP_SHARED, ifd, 0);
		if ((caddr_t)ptr == (caddr_t)-1) {
			fprintf (stderr, "%s: Can't read %s: %s\n",
				cmdname, imagefile, strerror(errno));
			exit (EXIT_FAILURE);
		}

		/*
		 * create copy of header so that we can blank out the
		 * checksum field for checking - this can't be done
		 * on the PROT_READ mapped data.
		 */
		memcpy (hdr, ptr, sizeof(image_header_t));

		if (ntohl(hdr->ih_magic) != IH_MAGIC) {
			fprintf (stderr,
				"%s: Bad Magic Number: \"%s\" is no valid image\n",
				cmdname, imagefile);
			exit (EXIT_FAILURE);
		}

		data = (char *)hdr;
		len  = sizeof(image_header_t);

		checksum = ntohl(hdr->ih_hcrc);
		hdr->ih_hcrc = htonl(0);	/* clear for re-calculation */

		if (crc32 (0, data, len) != checksum) {
			fprintf (stderr,
				"*** Warning: \"%s\" has bad header checksum!\n",
				imagefile);
		}

		data = (char *)(ptr + sizeof(image_header_t));
		len  = sbuf.st_size - sizeof(image_header_t) ;

		if (crc32 (0, data, len) != ntohl(hdr->ih_dcrc)) {
			fprintf (stderr,
				"*** Warning: \"%s\" has corrupted data!\n",
				imagefile);
		}

		/* for multi-file images we need the data part, too */
		print_header ((image_header_t *)ptr);

		(void) munmap((void *)ptr, sbuf.st_size);
		(void) close (ifd);

		exit (EXIT_SUCCESS);
	}

	/*
	 * Must be -w then:
	 *
	 * write dummy header, to be fixed later
	 */
	memset (hdr, 0, sizeof(image_header_t));

	if (write(ifd, hdr, sizeof(image_header_t)) != sizeof(image_header_t)) {
		fprintf (stderr, "%s: Write error on %s: %s\n",
			cmdname, imagefile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	if (opt_type == IH_TYPE_MULTI || opt_type == IH_TYPE_SCRIPT) {
		char *file = datafile;
		unsigned long size;

		for (;;) {
			char *sep = NULL;

			if (file) {
				if ((sep = strchr(file, ':')) != NULL) {
					*sep = '\0';
				}

				if (stat (file, &sbuf) < 0) {
					fprintf (stderr, "%s: Can't stat %s: %s\n",
						cmdname, file, strerror(errno));
					exit (EXIT_FAILURE);
				}
				size = htonl(sbuf.st_size);
			} else {
				size = 0;
			}

			if (write(ifd, (char *)&size, sizeof(size)) != sizeof(size)) {
				fprintf (stderr, "%s: Write error on %s: %s\n",
					cmdname, imagefile, strerror(errno));
				exit (EXIT_FAILURE);
			}

			if (!file) {
				break;
			}

			if (sep) {
				*sep = ':';
				file = sep + 1;
			} else {
				file = NULL;
			}
		}

		file = datafile;

		for (;;) {
			char *sep = strchr(file, ':');
			if (sep) {
				*sep = '\0';
				copy_file (ifd, file, 1);
				*sep++ = ':';
				file = sep;
			} else {
				copy_file (ifd, file, 0);
				break;
			}
		}
	} else {
		copy_file (ifd, datafile, 0);
	}

	/* We're a bit of paranoid */
#if defined(_POSIX_SYNCHRONIZED_IO) && !defined(__sun__) && !defined(__FreeBSD__)
	(void) fdatasync (ifd);
#else
	(void) fsync (ifd);
#endif

	if (fstat(ifd, &sbuf) < 0) {
		fprintf (stderr, "%s: Can't stat %s: %s\n",
			cmdname, imagefile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	ptr = (unsigned char *)mmap(0, sbuf.st_size,
				    PROT_READ|PROT_WRITE, MAP_SHARED, ifd, 0);
	if (ptr == (unsigned char *)MAP_FAILED) {
		fprintf (stderr, "%s: Can't map %s: %s\n",
			cmdname, imagefile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	hdr = (image_header_t *)ptr;

	checksum = crc32 (0,
			  (const char *)(ptr + sizeof(image_header_t)),
			  sbuf.st_size - sizeof(image_header_t)
			 );

	/* Build new header */
	hdr->ih_magic = htonl(IH_MAGIC);
	hdr->ih_time  = htonl(sbuf.st_mtime);
	hdr->ih_size  = htonl(sbuf.st_size - sizeof(image_header_t));
	hdr->ih_load  = htonl(addr);
	hdr->ih_ep    = htonl(ep);
	hdr->ih_dcrc  = htonl(checksum);
	hdr->ih_os    = opt_os;
	hdr->ih_arch  = opt_arch;
	hdr->ih_type  = opt_type;
	hdr->ih_comp  = opt_comp;

	if(vargv==0)
	{
		strncpy((char *)hdr->u.ih_name, name, IH_NMLEN);
	}
	else
	{
		strncpy((char *)tail_pre.productid, name, MAX_STRING);

#ifdef TRX_NEW
		linux_offset = rfs_offset / 2 ;  
		lrand = *(ptr + linux_offset); //get kernel data
	//printf("rfs_offset = %02x  lrand = %02x linux_offset=%02x\n", rfs_offset, lrand, linux_offset);	
		rootfs_offset =  rfs_offset + ((sbuf.st_size  - rfs_offset) / 2 );
		rrand = *(ptr + rootfs_offset);	//get rootfs data
	//printf("rfs_offset = %02x  rrand = %02x  hdr->ih_size = %02x rootfs_offset=%02x\n", rfs_offset, rrand, sbuf.st_size - sizeof(image_header_t), rootfs_offset);		
	
	tail_pre.pkey = lrand; 

	if (rrand== 0x0)
		tail_pre.key = 0xfd + lrand % 3;
	else
		tail_pre.key = 0xff - rrand + lrand;

	//printf ("Pkey:         %02x\n", tail_pre.pkey);
	//printf ("Key:          %02x\n", tail_pre.key);     
#endif	
			tail_pre.hw[0].major = lrand + rrand;
			tail_pre.hw[0].minor = lrand + 0xa1;
			tail_pre.hw[1].major = 2 * lrand + rrand;
			tail_pre.hw[1].minor = lrand + 0xb2;
			tail_pre.hw[2].major = 3 * lrand + rrand;
			tail_pre.hw[2].minor = lrand + 0xc3;

		memcpy(&hdr->u.tail, &tail_pre, sizeof(TAIL));

		if (rfs_offset) {
#ifdef TRX_NEW	

			version_t *v1 = &hdr->u.tail.hw[MAX_VER - 2], *v2 = v1+1;
#else
			version_t *v1 = &hdr->u.tail.hw[MAX_VER*2 - 2], *v2 = v1+1;
#endif			
			union {
				uint32_t rfs_offset_net_endian;
				uint8_t p[4];
			} u;
			u.rfs_offset_net_endian = htonl(rfs_offset);

			if (v1->major || v1->minor || v2->major || v2->minor)
				printf("Hardware Compatible List: %d.%d and %d.%d "
					"are override by rootfilesystem offset.\n",
					v1->major, v1->minor, v2->major, v2->minor);
			v1->major = ROOTFS_OFFSET_MAGIC;
			v1->minor = u.p[1];
			v2->major = u.p[2];
			v2->minor = u.p[3];
		}
	}

	checksum = crc32(0,(const char *)hdr,sizeof(image_header_t));

	hdr->ih_hcrc = htonl(checksum);

	print_header (hdr);

	(void) munmap((void *)ptr, sbuf.st_size);

	/* We're a bit of paranoid */
#if defined(_POSIX_SYNCHRONIZED_IO) && !defined(__sun__) && !defined(__FreeBSD__)
	(void) fdatasync (ifd);
#else
	(void) fsync (ifd);
#endif

	if (close(ifd)) {
		fprintf (stderr, "%s: Write error on %s: %s\n",
			cmdname, imagefile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	exit (EXIT_SUCCESS);
}

static void
copy_file (int ifd, const char *datafile, int pad)
{
	int dfd;
	struct stat sbuf;
	unsigned char *ptr;
	int tail;
	int zero = 0;
	int offset = 0;
	int size;

	if (vflag) {
		fprintf (stderr, "Adding Image %s\n", datafile);
	}

	if ((dfd = open(datafile, O_RDONLY|O_BINARY)) < 0) {
		fprintf (stderr, "%s: Can't open %s: %s\n",
			cmdname, datafile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	if (fstat(dfd, &sbuf) < 0) {
		fprintf (stderr, "%s: Can't stat %s: %s\n",
			cmdname, datafile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	ptr = (unsigned char *)mmap(0, sbuf.st_size,
				    PROT_READ, MAP_SHARED, dfd, 0);
	if (ptr == (unsigned char *)MAP_FAILED) {
		fprintf (stderr, "%s: Can't read %s: %s\n",
			cmdname, datafile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	if (xflag) {
		unsigned char *p = NULL;
		/*
		 * XIP: do not append the image_header_t at the
		 * beginning of the file, but consume the space
		 * reserved for it.
		 */

		if ((unsigned)sbuf.st_size < sizeof(image_header_t)) {
			fprintf (stderr,
				"%s: Bad size: \"%s\" is too small for XIP\n",
				cmdname, datafile);
			exit (EXIT_FAILURE);
		}

		for (p=ptr; p < ptr+sizeof(image_header_t); p++) {
			if ( *p != 0xff ) {
				fprintf (stderr,
					"%s: Bad file: \"%s\" has invalid buffer for XIP\n",
					cmdname, datafile);
				exit (EXIT_FAILURE);
			}
		}

		offset = sizeof(image_header_t);
	}

	size = sbuf.st_size - offset;
	if (write(ifd, ptr + offset, size) != size) {
		fprintf (stderr, "%s: Write error on %s: %s\n",
			cmdname, imagefile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	if (pad && ((tail = size % 4) != 0)) {

		if (write(ifd, (char *)&zero, 4-tail) != 4-tail) {
			fprintf (stderr, "%s: Write error on %s: %s\n",
				cmdname, imagefile, strerror(errno));
			exit (EXIT_FAILURE);
		}
	}

	(void) munmap((void *)ptr, sbuf.st_size);
	(void) close (dfd);
}

void
usage ()
{
	fprintf (stderr, "Usage: %s -l image\n"
			 "          -l ==> list image header information\n"
			 "       %s [-x] -A arch -O os -T type -C comp "
			 "-a addr -e ep -n name -d data_file[:data_file...] image\n",
		cmdname, cmdname);
	fprintf (stderr, "          -A ==> set architecture to 'arch'\n"
			 "          -O ==> set operating system to 'os'\n"
			 "          -T ==> set image type to 'type'\n"
			 "          -C ==> set compression type 'comp'\n"
			 "          -a ==> set load address to 'addr' (hex)\n"
			 "          -e ==> set entry point to 'ep' (hex)\n"
			 "          -n ==> set image name to 'name'\n"
			 "          -d ==> use image data from 'datafile'\n"
			 "          -x ==> set XIP (execute in place)\n"
		);
	exit (EXIT_FAILURE);
}

static void
print_header (image_header_t *hdr)
{
	time_t timestamp;
	uint32_t size;
	int i;
	uint32_t rfs_offset;

	timestamp = (time_t)ntohl(hdr->ih_time);
	size = ntohl(hdr->ih_size);

	if(vargv==0)
	{
		printf ("Image Name:   %.*s\n", IH_NMLEN, hdr->u.ih_name);
	}
	else
	{
		printf ("Product ID:   %.*s\n", MAX_STRING, hdr->u.tail.productid);
	}
	printf ("Image Type:   "); print_type(hdr);
	printf ("Data Size:    %d Bytes = %.2f kB = %.2f MB\n",
		size, (double)size / 1.024e3, (double)size / 1.048576e6 );
	printf ("Load Address: 0x%08X\n", ntohl(hdr->ih_load));
	printf ("Entry Point:  0x%08X\n", ntohl(hdr->ih_ep));

	if(vargv==1)
	{
		version_t *hw = &(hdr->u.tail.hw[0]);
		union {
			uint32_t rfs_offset_net_endian;
			uint8_t p[4];
		} u;
		rfs_offset = u.rfs_offset_net_endian = 0;

		printf ("Kernel Ver.:  %d.%d\n", hdr->u.tail.kernel.major, hdr->u.tail.kernel.minor);
		printf ("FS Ver:       %d.%d\n", hdr->u.tail.fs.major, hdr->u.tail.fs.minor);
		printf ("Hardware Compatible List:\n");
#ifdef TRX_NEW
		for(i=0; i<(MAX_VER); i++, hw++) 
#else
		for(i=0; i<(MAX_VER*2); i++, hw++) 
#endif	
		{		
			if (hw->major == ROOTFS_OFFSET_MAGIC) {
				u.p[1] = hw->minor;
				hw++;
				i++;
				u.p[2] = hw->major;
				u.p[3] = hw->minor;
				rfs_offset = ntohl(u.rfs_offset_net_endian);
			} else {
				printf("	%d: %d.%d\n", i+1, hw->major, hw->minor);
			}
		}

		printf ("Rootfilesystem offset:  0x%08X\n", rfs_offset);

	}
	if (hdr->ih_type == IH_TYPE_MULTI || hdr->ih_type == IH_TYPE_SCRIPT) {
		int i, ptrs;
		uint32_t pos;
		unsigned long *len_ptr = (unsigned long *) (
					(unsigned long)hdr + sizeof(image_header_t)
				);

		/* determine number of images first (to calculate image offsets) */
		for (i=0; len_ptr[i]; ++i)	/* null pointer terminates list */
			;
		ptrs = i;		/* null pointer terminates list */

		pos = sizeof(image_header_t) + ptrs * sizeof(long);
		printf ("Contents:\n");
		for (i=0; len_ptr[i]; ++i) {
			size = ntohl(len_ptr[i]);

			printf ("   Image %d: %8d Bytes = %4d kB = %d MB\n",
				i, size, size>>10, size>>20);
			if (hdr->ih_type == IH_TYPE_SCRIPT && i > 0) {
				/*
				 * the user may need to know offsets
				 * if planning to do something with
				 * multiple files
				 */
				printf ("    Offset = %08X\n", pos);
			}
			/* copy_file() will pad the first files to even word align */
			size += 3;
			size &= ~3;
			pos += size;
		}
	}
}


static void
print_type (image_header_t *hdr)
{
	printf ("%s %s %s (%s)\n",
		put_arch (hdr->ih_arch),
		put_os   (hdr->ih_os  ),
		put_type (hdr->ih_type),
		put_comp (hdr->ih_comp)
	);
}

static char *put_arch (int arch)
{
	return (put_table_entry(arch_name, "Unknown Architecture", arch));
}

static char *put_os (int os)
{
	return (put_table_entry(os_name, "Unknown OS", os));
}

static char *put_type (int type)
{
	return (put_table_entry(type_name, "Unknown Image", type));
}

static char *put_comp (int comp)
{
	return (put_table_entry(comp_name, "Unknown Compression", comp));
}

static char *put_table_entry (table_entry_t *table, char *msg, int type)
{
	for (; table->val>=0; ++table) {
		if (table->val == type)
			return (table->lname);
	}
	return (msg);
}

static int get_arch(char *name)
{
	return (get_table_entry(arch_name, "CPU", name));
}


static int get_comp(char *name)
{
	return (get_table_entry(comp_name, "Compression", name));
}


static int get_os (char *name)
{
	return (get_table_entry(os_name, "OS", name));
}


static int get_type(char *name)
{
	return (get_table_entry(type_name, "Image", name));
}

static int get_table_entry (table_entry_t *table, char *msg, char *name)
{
	table_entry_t *t;
	int first = 1;

	for (t=table; t->val>=0; ++t) {
		if (t->sname && strcasecmp(t->sname, name)==0)
			return (t->val);
	}
	fprintf (stderr, "\nInvalid %s Type - valid names are", msg);
	for (t=table; t->val>=0; ++t) {
		if (t->sname == NULL)
			continue;
		fprintf (stderr, "%c %s", (first) ? ':' : ',', t->sname);
		first = 0;
	}
	fprintf (stderr, "\n");
	return (-1);
}
