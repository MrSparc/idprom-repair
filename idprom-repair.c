/*
 *   Copyright (C) 02/2007 by Olaf Rempel
 *   razzor@kopf-tisch.de
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/random.h>

#include <asm/ebus.h>
#include <asm/idprom.h>

/* position of the idprom data */
#define IDPROM_POS	0x1FD8

/* position of the clock registers */
#define CLOCK_POS	0x1FF8

/* M48T59 has 8192 byte NVRAM */
#define NVRAM_END	0x1FFF

static struct resource * get_eeprom_resource(void)
{
	struct linux_ebus *ebus;
	struct linux_ebus_device *edev;

	for_each_ebus(ebus) {
		for_each_ebusdev(edev, ebus) {
			if (strcmp(edev->prom_node->name, "eeprom") == 0) {
				return &edev->resource[0];
			}
		}
	}
	return NULL;
}

static void init_nvram(struct resource *res)
{
	long addr;

	/* reset nvram-data, but do not overwrite the clock registers */
	for (addr = 0x0000; addr < CLOCK_POS; addr += 4)
		outl(0x0, res->start + addr);

	/* todo: kickstart clock? */
}

static void load_idprom_data(struct resource *res, struct idprom *data)
{
	long addr;
	for (addr = 0; addr < sizeof(struct idprom); addr++)
		*((u8 *)data + addr) = inb(res->start + IDPROM_POS + addr);
}

static void save_idprom_data(struct resource *res, struct idprom *data)
{
	long addr;
	for (addr = 0; addr < sizeof(struct idprom); addr++)
		outb(*((u8 *)data + addr), res->start + IDPROM_POS + addr);
}

/* Calculate the IDPROM checksum (xor of the data bytes). */
static unsigned char calc_idprom_cksum(struct idprom *idprom)
{
	unsigned char cksum, i, *ptr = (unsigned char *)idprom;

	for (i = cksum = 0; i <= 0x0E; i++)
		cksum ^= *ptr++;

	return cksum;
}

static void show_idprom(struct idprom *data)
{
	if (data->id_format != 0x01)
		printk(KERN_DEBUG "IDPROM: Warning, unknown format type!\n");

	if (data->id_cksum != calc_idprom_cksum(data))
		printk(KERN_DEBUG "IDPROM: Warning, checksum failure (nvram=%x, calc=%x)!\n",
		        data->id_cksum, calc_idprom_cksum(data));

	printk(KERN_DEBUG "IDPROM: HostID: %02x%02x%02x%02x Serial: %10d\n",
		data->id_machtype, data->id_ethaddr[3],
		data->id_ethaddr[4], data->id_ethaddr[5],
		data->id_sernum);

	printk(KERN_DEBUG "IDPROM: Ethernet address: %02x:%02x:%02x:%02x:%02x:%02x\n",
	        data->id_ethaddr[0], data->id_ethaddr[1],
		data->id_ethaddr[2], data->id_ethaddr[3],
		data->id_ethaddr[4], data->id_ethaddr[5]);
}

static void randomize_idprom(struct idprom *data)
{
	int sernum;
	memset(data, 0, sizeof(struct idprom));

	// sunblade 100
	data->id_format = 0x01;
	data->id_machtype = 0x83;

	// ethernet MAC
	get_random_bytes(data->id_ethaddr, 6);
	data->id_ethaddr[0] = 0x00;
	data->id_ethaddr[1] = 0x03;
	data->id_ethaddr[2] = 0xba;

	// serial number (4bytes after date)
	get_random_bytes(&sernum, sizeof(sernum));
	data->id_sernum = sernum;

	// xor checksum
	data->id_cksum = calc_idprom_cksum(data);
}

static int repair_init(void)
{
	struct idprom idprom_buffer;
	struct resource *res = get_eeprom_resource();
	if (res) {
		printk(KERN_DEBUG "found eeprom on ebus: addr: 0x%08lX - 0x%08lX\n",
			res->start, res->end);
	} else {
		printk(KERN_DEBUG "no eeprom resource found!\n");
		return -1;
	}

	load_idprom_data(res, &idprom_buffer);
	randomize_idprom(&idprom_buffer);
	show_idprom(&idprom_buffer);

	init_nvram(res);
	save_idprom_data(res, &idprom_buffer);
	return -1;
}

static void repair_cleanup(void)
{
}

module_init(repair_init);
module_exit(repair_cleanup);
MODULE_DESCRIPTION("Repairs SUN Blade 100 IDPROM");
MODULE_AUTHOR("(C) 2007 Olaf Rempel <razzor@kopf-tisch.de>");
MODULE_LICENSE("GPL");
