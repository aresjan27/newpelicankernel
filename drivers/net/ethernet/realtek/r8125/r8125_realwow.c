/*
################################################################################
#
# r8125 is the Linux device driver released for Realtek 2.5Gigabit Ethernet
# controllers with PCI-Express interface.
#
# Copyright(c) 2019 Realtek Semiconductor Corp. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, see <http://www.gnu.org/licenses/>.
#
# Author:
# Realtek NIC software team <nicfae@realtek.com>
# No. 2, Innovation Road II, Hsinchu Science Park, Hsinchu 300, Taiwan
#
################################################################################
*/

/************************************************************************************
 *  This product is covered by one or more of the following patents:
 *  US6,570,884, US6,115,776, and US6,327,625.
 ***********************************************************************************/

#include <linux/module.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/delay.h>
#include <linux/in.h>
#include <linux/ethtool.h>

#include "r8125.h"
#include "r8125_realwow.h"
#include "rtl_eeprom.h"

#ifdef CONFIG_ARCH_RTD129x
extern void (*pcie_write)(u32 addr, u8 size, u32 wval);
extern uint32_t (*pcie_read)(u32 addr, u8 size);
#endif

void rtl8125_realwow_hw_init(struct net_device *dev)
{
        struct rtl8125_private *tp = netdev_priv(dev);

        if ( HW_SUPPORT_KCP_OFFLOAD( tp ) ) {
                rtl8125_wait_txrx_fifo_empty(dev);

                tp->EnableKCPOffload= FALSE;
                tp->EnableDhcpTimeoutWake = FALSE;

                if ( tp->HwSuppKCPOffloadVer == 6) {
                        u32 TmpUlong;
                        //Clear previous ack miss enable
                        RTL_W32(tp, MACOCP, 0xE9720000);

                        //Configure teredo disable
                        //0xE05E_0000
                        RTL_W32(tp, MACOCP, 0x605E0000);
                        TmpUlong = RTL_R32(tp, MACOCP);
                        if (tp->HwSuppKCPOffloadVer == 6) {
                                TmpUlong &=  0x7EFF; //clear Bit_8 Bit_15
                        }
                        RTL_W32(tp, MACOCP, (0xE05E << 16) | TmpUlong);

                        //21. timer turn off
                        //0xF214_0010
                        RTL_W32(tp, MACOCP, 0xF2140010);

                        //clear DHCP real timer
                        RTL_W32(tp, MACOCP, 0xE9660000);
                        RTL_W32(tp, MACOCP, 0xE9670000);
                        //configure DHCP expired timer
                        RTL_W32(tp, MACOCP, 0xE9640000);
                        RTL_W32(tp, MACOCP, 0xE9650000);
                }
        }
}

void rtl8125_get_realwow_hw_version(struct net_device *dev)
{
        struct rtl8125_private *tp = netdev_priv(dev);

        switch (tp->mcfg) {
        case CFG_METHOD_2:
        case CFG_METHOD_3:
        case CFG_METHOD_4:
        case CFG_METHOD_5:
                tp->HwSuppKCPOffloadVer = 6;
                break;
        }
}

static void DisableMcu(struct rtl8125_private *tp)
{
        u16 TmpUshort;

        switch(tp->mcfg) {
        case CFG_METHOD_2:
        case CFG_METHOD_3:
        case CFG_METHOD_4:
        case CFG_METHOD_5:
                TmpUshort = rtl8125_mac_ocp_read(tp, 0xC0B4);
                TmpUshort &= ~(BIT_0);
                rtl8125_mac_ocp_write(tp, 0xC0B4, TmpUshort);
                break;
        }
}

static void EnableMcu(struct rtl8125_private *tp)
{
        u16 TmpUshort;

        switch(tp->mcfg) {
        case CFG_METHOD_2:
        case CFG_METHOD_3:
        case CFG_METHOD_4:
        case CFG_METHOD_5:
                TmpUshort = rtl8125_mac_ocp_read(tp, 0xC0B4);
                TmpUshort |= (BIT_0);
                rtl8125_mac_ocp_write(tp, 0xC0B4, TmpUshort);
                break;
        }
}

//TCAM
#define TCAM_NOTVALID_ADDR 0xA000
#define TCAM_VALID_ADDR 0xA800
static void NICTcamWrite(
        struct rtl8125_private *tp,
        bool bNotValid,
        u16 offset,
        u16 data,
        u16 carebit)
{
        u16 TcamAddr;
        u32 TcamData;

        if (bNotValid) {
                TcamAddr = TCAM_NOTVALID_ADDR;
        } else {
                TcamAddr = TCAM_VALID_ADDR;
        }
        TcamAddr += offset * 4;

        TcamData = carebit;
        TcamData <<= 16;
        TcamData += data;

        RTL_W32(tp, TcamAddr, TcamData);
}

static void InitKCPOffload(struct net_device *dev)
{
        struct rtl8125_private *tp = netdev_priv(dev);

        if ( HW_SUPPORT_KCP_OFFLOAD( tp ) ) {

                u8 TmpUchar;
                u32 tmpUlong;
                u16 tmpUshort;
                u16 i;

                TmpUchar = RTL_R8(tp, MCUCmd_reg);
                TmpUchar |= (BIT_0);
                RTL_W8(tp, MCUCmd_reg, TmpUchar);

                DisableMcu(tp);

                if (tp->HwSuppKCPOffloadVer == 6) {
                        //bool AldpsEnabled;

                        //AldpsEnabled = DisableAldpsMode_8125(Adapter);

                        //DisableMcu(tp);
                        //MACID vlan in TCAM
                        NICTcamWrite(tp, FALSE, 496, 0xFFFF, 0xFFFF); //set vlan id 0xFFFF
                        tmpUshort = *(u16*)&tp->MpKCPInfo.MacID[0];
                        NICTcamWrite(tp, FALSE, 448, tmpUshort, 0xFFFF);
                        tmpUshort = *(u16*)&tp->MpKCPInfo.MacID[2];
                        NICTcamWrite(tp, FALSE, 449, tmpUshort, 0xFFFF);
                        tmpUshort = *(u16*)&tp->MpKCPInfo.MacID[4];
                        NICTcamWrite(tp, FALSE, 450, tmpUshort, 0xFFFF);


                        //if (AldpsEnabled)
                        //EnableAldpsMode_8125(tp);

                        //arp mac id NS0_MACID : ERI offset 0x25d-0x258(OCP offset 0xCD75-0xCD70)
                        for (i=0; i<6; i+=2) {
                                tmpUshort = *(u16*)&tp->MpKCPInfo.MacID[i];
                                rtl8125_mac_ocp_write(tp, 0xCD70+i, tmpUshort);
                        }
                        //OOB_IPv4 : ERI offset 0x1f3-0x1f0(OCP offset 0xCD0B-0xCD08)
                        for (i=0; i<4; i+=2) {
                                tmpUshort = *(u16*)&tp->MpKCPInfo.DIPv4[i];
                                rtl8125_mac_ocp_write(tp, 0xCD08+i, tmpUshort);
                        }

                        // 1.clear previous wakeup type: OCP 0xC0C4 = 0x00FF
                        rtl8125_mac_ocp_write(tp, 0xC0C4, 0x00FF);

                        // 2. clear previous ack miss counter: OCP 0xD2E8 = 0x0000
                        rtl8125_mac_ocp_write(tp, 0xD2E8, 0x0000);

                        // 3.UDP port: OCP 0xCDB2 = SERVER1_PORT
                        // 4. UDP port: OCP 0xCDB4 = SERVER2_PORT
                        rtl8125_mac_ocp_write(tp, 0xCDB2, tp->MpKCPInfo.UdpPort[0]);
                        rtl8125_mac_ocp_write(tp, 0xCDB4, tp->MpKCPInfo.UdpPort[1]);

                        // 5. port enable: OCP 0xC0BE bit11=1, bit6=1, bit5=1
                        // 6. port enable: OCP 0xC0C0 bit6=1, bit5=1
                        tmpUshort = rtl8125_mac_ocp_read(tp, 0xC0BE);
                        tmpUshort |= (BIT_5|BIT_6|BIT_11);
                        rtl8125_mac_ocp_write(tp, 0xC0BE, tmpUshort);

                        tmpUshort = rtl8125_mac_ocp_read(tp, 0xC0C0);
                        tmpUshort |= (BIT_5|BIT_6);
                        rtl8125_mac_ocp_write(tp, 0xC0C0, tmpUshort);

                        // 7. rsvd_proxy_udp enable: OCP 0xC0B6 bit10=1
                        tmpUshort = rtl8125_mac_ocp_read(tp, 0xC0B6);
                        tmpUshort |= (BIT_10);
                        rtl8125_mac_ocp_write(tp, 0xC0B6, tmpUshort);

                        // 8.ack miss timer: OCP 0xD2E6 = ACKMISS_TIME
                        rtl8125_mac_ocp_write(tp, 0xD2E6, tp->MpKCPInfo.ackLostCnt);

                        // 9.Disable ftr_mcu_en(ERI 0xDC(OCP 0xC0B4) [0] = 0)
                        //10. Enable ftr_mcu_en(ERI 0xDC(OCP 0xC0B4) [0] = 1)

                        // 11. rx length: OCP 0x6800 = ACKLEN[7:0]_WAKELEN[7:0]
                        // 12. tx length: OCP 0x6802 = TX2LEN[7:0]_TX1LEN[7:0]
                        tmpUshort = 0x0000;
                        tmpUshort |= tp->MpKCPInfo.PKTLEN[0];
                        tmpUshort |= ((tp->MpKCPInfo.PKTLEN[1] << 8)& 0xFF00);
                        rtl8125_mac_ocp_write(tp, 0x6802, tmpUshort);

                        tmpUshort = tp->MpKCPInfo.KCP_WakePattern_Len;
                        tmpUshort |= tp->MpKCPInfo.KCP_AckPacket_Len << 8;
                        rtl8125_mac_ocp_write(tp, 0x6800, tmpUshort);

                        // 13. wake packet UDP payload (64 bytes): OCP 0x6808-6847
                        // 14. ack packet UDP payload (48 bytes): OCP 0x6848-6877
                        for (i=0; i<64; i+=2) {
                                tmpUshort = *(u16*)&tp->MpKCPInfo.KCP_WakePattern[i];
                                rtl8125_mac_ocp_write(tp, 0x6808+i, tmpUshort);
                        }
                        for (i=0; i<48; i+=2) {
                                tmpUshort = *(u16*)&tp->MpKCPInfo.KCP_AckPacket[i];
                                rtl8125_mac_ocp_write(tp, 0x6848+i, tmpUshort);
                        }

                        //15. tx1 whole packet excluding crc (100 bytes) : OCP 0x6880-68E3
                        //16. tx2 whole packet excluding crc (100 bytes) : OCP 0x68E4-6947
                        for (i=0; i<100; i+=2) {
                                tmpUshort = *(u16*)&tp->MpKCPInfo.KCP_TxPacket[0][i];
                                rtl8125_mac_ocp_write(tp, 0x6880+i, tmpUshort);
                        }
                        for (i=0; i<100; i+=2) {
                                tmpUshort = *(u16*)&tp->MpKCPInfo.KCP_TxPacket[1][i];
                                rtl8125_mac_ocp_write(tp, 0x68E4+i, tmpUshort);
                        }

                        //17. ack miss wakeup enable/disable : OCP 0xD2E4 bit 0
                        if (tp->MpKCPInfo.ackLostCnt>0) {
                                rtl8125_mac_ocp_write(tp, 0xD2E4, 0x0001);
                        } else {
                                rtl8125_mac_ocp_write(tp, 0xD2E4, 0x0000);
                        }

                        //18. RealWOWv2  enable/disable : OCP 0xC0BC = 0x0100
                        rtl8125_mac_ocp_write(tp, 0xC0BC, 0x0100);

                        //19. tx packet timer (in units of 8ms): OCP 0xE42A = TX_TIMER
                        rtl8125_mac_ocp_write(tp, 0xE42A, 0x0700);

                        //20. tx packet timer enable: OCP 0xE428 = 0x0019
                        rtl8125_mac_ocp_write(tp, 0xE428, 0x0019);

                        tmpUlong = (tp->DhcpTimeout*1000)/tp->MpKCPInfo.KCP_interval;
                        if (tp->EnableDhcpTimeoutWake) {
                                //16-9. Clear previous DHCP real timer :
                                // 1. OCP 0xD2CC = 0x0000
                                // 2. OCP 0xD2CE = 0x0000
                                rtl8125_mac_ocp_write(tp, 0xD2CC, 0x0000);
                                rtl8125_mac_ocp_write(tp, 0xD2CE, 0x0000);

                                //16-10. Configure DHCP expire timer (DHCPTIME is the duration of each tx, 0 is disable) :
                                //  1. OCP 0xD2C8 = DHCPTIME[15:0]
                                //  2. OCP 0xD2C8 = DHCPTIME[31:16]  (if transmit tx every 15 sec and DHCP timeou is 30 sec, then DHCPTIME is 2)

                                rtl8125_mac_ocp_write(tp, 0xD2C8, (u16) tmpUlong);
                                rtl8125_mac_ocp_write(tp, 0xD2CA, (u16)(tmpUlong>>16));

                        } else {
                                //16-9. Clear previous DHCP real timer :
                                // 1. OCP 0xD2CC = 0x0000
                                // 2. OCP 0xD2CE = 0x0000
                                rtl8125_mac_ocp_write(tp, 0xD2CC, 0x0000);
                                rtl8125_mac_ocp_write(tp, 0xD2CE, 0x0000);

                                //16-10. Configure DHCP expire timer (DHCPTIME is the duration of each tx, 0 is disable) :
                                //  1. OCP 0xD2C8 = DHCPTIME[15:0]
                                //  2. OCP 0xD2C8 = DHCPTIME[31:16]  (if transmit tx every 15 sec and DHCP timeou is 30 sec, then DHCPTIME)
                                rtl8125_mac_ocp_write(tp, 0xD2C8, 0x0000);
                                rtl8125_mac_ocp_write(tp, 0xD2CA, 0x0000);
                        }
                }

                EnableMcu(tp);
        }
}

void rtl8125_set_realwow_d3_para(struct net_device *dev)
{
        struct rtl8125_private *tp = netdev_priv(dev);

        if( tp->EnableKCPOffload ) {
                switch (tp->mcfg) {
                case CFG_METHOD_2:
                case CFG_METHOD_3:
                case CFG_METHOD_4:
                case CFG_METHOD_5:
                        rtl8125_mac_ocp_write(tp, 0xC0F6, 0x0000);

                        rtl8125_mac_ocp_write(tp, 0xC0AA, 0x0100);
                        rtl8125_mac_ocp_write(tp, 0xC0A6, 0x0040);

                        rtl8125_disable_now_is_oob(tp);
                        break;
                }
        }

        if ( HW_SUPPORT_KCP_OFFLOAD(tp) ) {
                if( tp->EnableKCPOffload) {
                        InitKCPOffload(dev);
                } else {
                        // 3 clear wakeup event
                        if ( tp->HwSuppKCPOffloadVer == 6) {
                                //clear previous wakeup type: OCP 0xC0C4 = 0x00FF
                                RTL_W32(tp, MACOCP, 0xE06200FF);
                        }
                }
        }

        if( tp->EnableKCPOffload ) {
                u16 data16;

                DisableMcu(tp);
                EnableMcu(tp);

                rtl8125_enable_now_is_oob( tp );

                switch (tp->mcfg) {
                case CFG_METHOD_2:
                case CFG_METHOD_3:
                case CFG_METHOD_4:
                case CFG_METHOD_5:
                        data16 = rtl8125_mac_ocp_read(tp, 0xE8DE) | BIT_14;
                        rtl8125_mac_ocp_write(tp, 0xE8DE, data16);
                        break;
                }
        }

        if ( tp->EnableKCPOffload ) {
                u32 tmplong;

                if( tp->HwSuppKCPOffloadVer == 6) {
                        tmplong=(tp->MpKCPInfo.KCP_interval/8)-10;
                        tmplong |=0xF2150000;

                        RTL_W32(tp, MACOCP, tmplong);
                        RTL_W32(tp, MACOCP, 0xF2140019);
                }
        }
}

int rtl8125_realwow_ioctl(struct net_device *dev, struct ifreq *ifr)
{
        struct rtl8125_private *tp = netdev_priv(dev);
        void *user_data = ifr->ifr_data;
        struct rtl_realwow_ioctl_struct rtl_realwow_usrdata;
        unsigned long flags;
        int ret=0;


        if (FALSE == HW_SUPPORT_KCP_OFFLOAD(tp))
                return -EOPNOTSUPP;


        if (copy_from_user(&rtl_realwow_usrdata, user_data, sizeof(struct rtl_realwow_ioctl_struct)))
                return -EFAULT;

        switch (rtl_realwow_usrdata.cmd) {
        case RTL_REALWOW_SET_KCP_DISABLE:
                if (!capable(CAP_NET_ADMIN))
                        return -EPERM;

                spin_lock_irqsave(&tp->lock, flags);
                tp->EnableKCPOffload = FALSE;
                spin_unlock_irqrestore(&tp->lock, flags);
                break;
        case RTL_REALWOW_SET_KCP_INFO:
                if (!capable(CAP_NET_ADMIN))
                        return -EPERM;

                if (rtl_realwow_usrdata.len < sizeof(KCPInfo))
                        return -EINVAL;

                {
                        PKCPInfo pKCPInfo;

                        if (!(pKCPInfo = kmalloc(rtl_realwow_usrdata.len, GFP_KERNEL)))
                                return -ENOMEM;

                        if (copy_from_user(pKCPInfo, rtl_realwow_usrdata.data_buffer, rtl_realwow_usrdata.len)) {
                                kfree(pKCPInfo);
                                return -EFAULT;
                        }

                        spin_lock_irqsave(&tp->lock, flags);

                        //IP
                        memcpy( tp->MpKCPInfo.DIPv4 , pKCPInfo->DIPv4, 4 );
                        //MAC
                        memcpy( tp->MpKCPInfo.MacID, pKCPInfo->MacID, 6 );

                        if( pKCPInfo->nId == 0 ) {
                                tp->MpKCPInfo.UdpPort[0] = pKCPInfo->UdpPort;
                                tp->MpKCPInfo.PKTLEN[0] = (u8)pKCPInfo->PKTLEN;
                        } else {
                                tp->MpKCPInfo.UdpPort[1] = pKCPInfo->UdpPort;
                                tp->MpKCPInfo.PKTLEN[1] = (u8)pKCPInfo->PKTLEN;
                        }

                        spin_unlock_irqrestore(&tp->lock, flags);

                        kfree(pKCPInfo);
                }
                break;
        case RTL_REALWOW_SET_KCP_CONTENT:
                if (!capable(CAP_NET_ADMIN))
                        return -EPERM;

                if (rtl_realwow_usrdata.len < sizeof(KCPContent))
                        return -EINVAL;

                {
                        PKCPContent pKCP;

                        if (!(pKCP = kmalloc(rtl_realwow_usrdata.len, GFP_KERNEL)))
                                return -ENOMEM;

                        if (copy_from_user(pKCP, rtl_realwow_usrdata.data_buffer, rtl_realwow_usrdata.len)) {
                                kfree(pKCP);
                                return -EFAULT;
                        }

                        if ( pKCP->size> MAX_RealWoW_KCP_SIZE ) {
                                kfree(pKCP);
                                return -EOPNOTSUPP;
                        }

                        spin_lock_irqsave(&tp->lock, flags);

                        if (tp->EnableTeredoOffload == TRUE) {
                                spin_unlock_irqrestore(&tp->lock, flags);
                                kfree(pKCP);
                                return -EOPNOTSUPP;
                        }

                        tp->MpKCPInfo.KCP_interval=pKCP->mSec;

                        if(pKCP->id==0) {
                                memcpy( tp->MpKCPInfo.KCP_TxPacket[0], pKCP->bPacket, pKCP->size);
                                tp->MpKCPInfo.PKTLEN[0]= (u8)pKCP->size;
                        } else {
                                memcpy( tp->MpKCPInfo.KCP_TxPacket[1],  pKCP->bPacket, pKCP->size);
                                tp->MpKCPInfo.PKTLEN[1]= (u8)pKCP->size;
                        }

                        tp->EnableKCPOffload = TRUE;

                        spin_unlock_irqrestore(&tp->lock, flags);

                        kfree(pKCP);
                }
                break;
        case RTL_REALWOW_SET_KCP_ACKPKTINFO:
                if (!capable(CAP_NET_ADMIN))
                        return -EPERM;

                if (rtl_realwow_usrdata.len < sizeof(RealWoWAckPktInfo))
                        return -EINVAL;

                {
                        PRealWoWAckPktInfo pAckPktInfo;

                        if (!(pAckPktInfo = kmalloc(rtl_realwow_usrdata.len, GFP_KERNEL)))
                                return -ENOMEM;

                        if (copy_from_user(pAckPktInfo, rtl_realwow_usrdata.data_buffer, rtl_realwow_usrdata.len)) {
                                kfree(pAckPktInfo);
                                return -EFAULT;
                        }

                        spin_lock_irqsave(&tp->lock, flags);


                        memcpy( tp->MpKCPInfo.KCP_AckPacket,pAckPktInfo->pattern, pAckPktInfo->patterntSize);
                        tp->MpKCPInfo.KCP_AckPacket_Len=(u8)pAckPktInfo->patterntSize;
                        tp->MpKCPInfo.ackLostCnt=pAckPktInfo->ackLostCnt;

                        spin_unlock_irqrestore(&tp->lock, flags);

                        kfree(pAckPktInfo);
                }
                break;
        case RTL_REALWOW_SET_KCP_WPINFO:
                if (!capable(CAP_NET_ADMIN))
                        return -EPERM;

                if (rtl_realwow_usrdata.len < sizeof(RealWoWWPInfo))
                        return -EINVAL;

                {
                        PRealWoWWPInfo pWPInfo;

                        if (!(pWPInfo = kmalloc(rtl_realwow_usrdata.len, GFP_KERNEL)))
                                return -ENOMEM;

                        if (copy_from_user(pWPInfo, rtl_realwow_usrdata.data_buffer, rtl_realwow_usrdata.len)) {
                                kfree(pWPInfo);
                                return -EFAULT;
                        }

                        spin_lock_irqsave(&tp->lock, flags);

                        memcpy( tp->MpKCPInfo.KCP_WakePattern, pWPInfo->pattern, pWPInfo->patterntSize);
                        tp->MpKCPInfo.KCP_WakePattern_Len=(u8)pWPInfo->patterntSize;

                        spin_unlock_irqrestore(&tp->lock, flags);

                        kfree(pWPInfo);
                }
                break;
        case RTL_REALWOW_SET_KCPDHCP_TIMEOUT:
                if (!capable(CAP_NET_ADMIN))
                        return -EPERM;

                if (rtl_realwow_usrdata.len < sizeof(u32))
                        return -EINVAL;

                spin_lock_irqsave(&tp->lock, flags);

                tp->EnableDhcpTimeoutWake = TRUE;

                memcpy( &tp->DhcpTimeout, rtl_realwow_usrdata.data_buffer, sizeof(u32) );

                spin_unlock_irqrestore(&tp->lock, flags);
                break;
        default:
                return -EOPNOTSUPP;
        }


        return ret;
}
