/*
 * DiskSim Storage Subsystem Simulation Environment (Version 4.0)
 * Revision Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2001-2008.
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to reproduce, use, and prepare derivative works of this
 * software is granted provided the copyright and "No Warranty" statements
 * are included with all reproductions and derivative works and associated
 * documentation. This software may also be redistributed without charge
 * provided that the copyright and "No Warranty" statements are included
 * in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT
 * TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
 * COPYRIGHT HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE
 * OR DOCUMENTATION.
 *
 */



/*
 * DiskSim Storage Subsystem Simulation Environment (Version 2.0)
 * Revision Authors: Greg Ganger
 * Contributors: Ross Cohen, John Griffin, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 1999.
 *
 * Permission to reproduce, use, and prepare derivative works of
 * this software for internal use is granted provided the copyright
 * and "No Warranty" statements are included with all reproductions
 * and derivative works. This software may also be redistributed
 * without charge provided that the copyright and "No Warranty"
 * statements are included in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT
 * TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
 */

/*
 * DiskSim Storage Subsystem Simulation Environment
 * Authors: Greg Ganger, Bruce Worthington, Yale Patt
 *
 * Copyright (C) 1993, 1995, 1997 The Regents of the University of Michigan 
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose and without fee or royalty is
 * hereby granted, provided that the full text of this NOTICE appears on
 * ALL copies of the software and documentation or portions thereof,
 * including modifications, that you make.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY OF EXAMPLE,
 * BUT NOT LIMITATION, COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS OR
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR
 * THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY
 * THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS. COPYRIGHT
 * HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE OR
 * DOCUMENTATION.
 *
 *  This software is provided AS IS, WITHOUT REPRESENTATION FROM THE
 * UNIVERSITY OF MICHIGAN AS TO ITS FITNESS FOR ANY PURPOSE, AND
 * WITHOUT WARRANTY BY THE UNIVERSITY OF MICHIGAN OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS
 * OF THE UNIVERSITY OF MICHIGAN SHALL NOT BE LIABLE FOR ANY DAMAGES,
 * INCLUDING SPECIAL , INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES,
 * WITH RESPECT TO ANY CLAIM ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OF OR IN CONNECTION WITH THE USE OF THE SOFTWARE, EVEN IF IT HAS
 * BEEN OR IS HEREAFTER ADVISED OF THE POSSIBILITY OF SUCH DAMAGES
 *
 * The names and trademarks of copyright holders or authors may NOT be
 * used in advertising or publicity pertaining to the software without
 * specific, written prior permission. Title to copyright in this software
 * and any associated documentation will at all times remain with copyright
 * holders.
 */


#include "disksim_global.h"
#include "disksim_stat.h"
#include "disksim_iosim.h"
#include "disksim_iotrace.h"
#include "disksim_iodriver.h"
#include "disksim_logorg.h"
#include "disksim_orgface.h"
#include "disksim_ioqueue.h"
#include "disksim_bus.h"
#include "disksim_controller.h"
#include "config.h"

#include "modules/modules.h"

#include "../ssdmodel/ssd_dedupRAID.h"
#include "../ssdmodel/ssd.h"

// These codes are negelected

event * iodriver_request (int iodriverno, ioreq_event *curr)
{
   ioreq_event *temp = NULL;
   ioreq_event *ret = NULL;
   ioreq_event *retlist = NULL;
   int numreqs;
/*
   printf ("Entered iodriver_request - simtime %f, devno %d, blkno %d, cause %d\n", simtime, curr->devno, curr->blkno, curr->cause);
   fprintf (outputfile, "Entered iodriver_request - simtime %f, devno %d, blkno %d, cause %d\n", simtime, curr->devno, curr->blkno, curr->cause);
   fprintf (stderr, "Entered iodriver_request - simtime %f, devno %d, blkno %d, cause %d\n", simtime, curr->devno, curr->blkno, curr->cause);
*/
   if (outios) {
      fprintf(outios, "%.6f\t%d\t%d\t%d\t%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags);
   }

#if 0
   fprintf (stderr, "Entered iodriver_request - simtime %f, devno %d, blkno %d, cause %d\n", simtime, curr->devno, curr->blkno, curr->cause);
#endif

   /* add to the overall queue to start tracking */
   ret = ioreq_copy (curr);
   ioqueue_add_new_request (overallqueue, ret);
   ret = NULL;
 
   disksim->totalreqs++;
   if ((disksim->checkpoint_iocnt > 0) && ((disksim->totalreqs % disksim->checkpoint_iocnt) == 0)) {
      disksim_register_checkpoint (simtime);
   }
   // if (disksim->totalreqs == disksim->warmup_iocnt) {
   //    warmuptime = simtime;
   //    resetstats();
   // }

   // this is how I set my warm up
   if (logData->traceNum >= WARM_UP_TRACE && warmupset == 0){
      printf("now doing reset stateus after warm up\n");
      warmuptime = simtime;
      resetstats();
      warmupset = 1;
      //ASSERT(gctimes > 0);
      log_set_warmupGC(logData, gctimes);
      printf("after warm up, done gc %d times\n", gctimes);
   }

   // an indicator vairable for modification in disksim_redun.c
   BLKNO = curr->blkno;

   numreqs = logorg_maprequest(sysorgs, numsysorgs, curr);
   temp = curr->next;
   for (; numreqs>0; numreqs--) {
          /* Request list size must match numreqs */
      ASSERT(curr != NULL);
      curr->next = NULL;
      if ((iodrivers[iodriverno]->consttime == IODRIVER_TRACED_QUEUE_TIMES) || (iodrivers[iodriverno]->consttime == IODRIVER_TRACED_BOTH_TIMES)) {
         ret = ioreq_copy(curr);
         ret->time = simtime + (double) ret->tempint1 / (double) 1000;
         ret->type = IO_TRACE_REQUEST_START;
         addtointq((event *) ret);
         ret = NULL;
         if ((curr->slotno == 1) && (ioqueue_get_number_in_queue(iodrivers[iodriverno]->devices[(curr->devno)].queue) == 0)) {
            iodrivers[(iodriverno)]->devices[(curr->devno)].flag = 2;
            iodrivers[(iodriverno)]->devices[(curr->devno)].lastevent = simtime;
         }
      }
      ret = handle_new_request(iodrivers[iodriverno], curr);

      if ((ret) && (iodrivers[iodriverno]->type == STANDALONE) && (ret->time == 0.0)) {
         ret->type = IO_ACCESS_ARRIVE;
         ret->time = simtime;
         iodriver_schedule(iodriverno, ret);
      } else if (ret) {
         ret->type = IO_ACCESS_ARRIVE;
         ret->next = retlist;
         ret->prev = NULL;
         retlist = ret;
      }
      curr = temp;
      temp = (temp) ? temp->next : NULL;
   }
   if (iodrivers[iodriverno]->type == STANDALONE) {
      while (retlist) {
         ret = retlist;
         retlist = ret->next;
         ret->next = NULL;
         ret->time += simtime;
         addtointq((event *) ret);
      }
   }
/*
fprintf (outputfile, "leaving iodriver_request: retlist %p\n", retlist);
*/
   return((event *) retlist);
}

// other codes are negelected