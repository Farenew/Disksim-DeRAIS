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

#include "config.h"

#include "disksim_global.h"
#include "disksim_hptrace.h"
#include "disksim_iotrace.h"
#include "../ssdmodel/ssd_dedupRAID.h"


// Other codes are negelected 

// this is the entry for code, trace get read, deduplicated, controled by DeRAIS here. I treat this as the neck between operating system and storage system.

static int reqNum = 0;
static trace tr;

static ioreq_event * iotrace_ascii_get_ioreq_event (FILE *tracefile, ioreq_event *new)
{
   int lineLength = 0;
   char* line = NULL;
   int getLineSize = 0;
   char* hash;
   int groupNo = 0;

getRequest:

   if(reqNum == 0 && GC_TAG == NOT_GC){

      int bcount;
      if((lineLength = getline(&line, &getLineSize, tracefile)) == -1){
         addtoextraq((event*)new);
         return(NULL);
      }

      log_another_trace(logData, warmupset);

      if(logData->traceNum >= WARM_UP_TRACE && warmupset == 0){
         printf("now have done warm up, warm up with %d traces\n", WARM_UP_TRACE);
      }

      bcount = get_bcount(line, 3);
      // request must be page aligned
      ASSERT(bcount % PAGE_SIZE == 0);
      reqNum = bcount / PAGE_SIZE;

      if(reqNum == 0){
         free(line);
         line = NULL;
         goto getRequest;
      }

      tr.hash = (char*)malloc(sizeof(char) * HASH_SIZE * reqNum + 1);

      if (sscanf(line, "%lf %d %d %d %s\n", &tr.time, &tr.blkno, &tr.bcount, &tr.flags, tr.hash) != 5) {
         fprintf(stderr, "Wrong number of arguments for I/O trace event type\n");
         fprintf(stderr, "line: %s\n", line);
         ddbg_assert(0);
      }

      //tr.time /= 1000000;
      //tr.time /= 100;
      currentTime = tr.time;
      tr.hash[HASH_SIZE * reqNum] = '\0';

      tr.hashStart = tr.hash;
      free(line);
      line = NULL;

      log_trace_add(logData, tr.flags, warmupset);
      //printf("%f %d %d %d %s\n", tr.time, tr.blkno, tr.flags, tr.bcount, tr.hash);

   }

   if(GC_TAG == NOT_GC){
      
      ASSERT(reqNum != 0);
      reqNum --;
      
      get_ioreq(&tr, new, PAGE_SIZE);

      //printf("now get a new req blkno %d bcount %d hash %s\n", new->blkno, new->bcount, new->hash);
      log_print(logData);

      if(reqNum == 0){
         free(tr.hashStart);
         tr.hash = NULL;
         tr.hashStart = NULL;
      }
      
      new->blkno /= 8;
      new->blkno *= 8;

      new->ref = 1;
   }
   else{
      ASSERT(reqNum != 0);
      ASSERT(GClevel != EMPTY);
      workingGCpage = get_page_from_GC(GCstripe);
      new->ref = get_ref_from_page(workingGCpage);
      reqNum --;
      goto Write;
   }

   log_total_add(logData, new->flags, warmupset);
   
   //////////////////////////////////////////////
   // here is my architecture
   //////////////////////////////////////////////

   // if remap is -1, it means this hash doesn't exist in hashtable, 
   // else, it points to the hash where it stores
   if(new->flags == TRACE_WRITE){

      int remap = isInHashTable((char*)new->hash);

      if(remap != -1){
         set_page_map(new->blkno / PAGE_SIZE, remap, (char*)new->hash);
         free((char*)new->hash);
         new->hash = NULL;
         goto getRequest;
      }
      else{
Write:

         groupNo = find_group(new->ref);
         ASSERT(groupNo < GROUP_NUM);
         // # full stripe part
         if(workingStripe[groupNo] == EMPTY_STATE){
            workingStripe[groupNo] = find_stripe_from_group(groupNo);
            //workingStripe[groupNo] = find_stripe_to_write(FULL_WRITE);
            // this working stripe should not be gc stripe
            ASSERT(workingStripe[groupNo] != GCstripe);
         }
         ASSERT(workingStripe[groupNo] != EMPTY_STATE);

         int newPage = -1;
         if(fullStripe[groupNo] == EMPTY_STATE){
            newPage = find_empty_page_from_stripe(workingStripe[groupNo], FULL_WRITE);
            fullStripe[groupNo] = 1;
            writingPage = newPage;
         }
         else{
            newPage = find_page_from_stripe(workingStripe[groupNo], writingPage, NEXT);
            ASSERT(newPage != EMPTY);
            writingPage = newPage;
            fullStripe[groupNo] ++;
         }
         // # end of full stripe part

         if(GC_TAG == IN_GC){
            // this move operation contains set_map, cancel_map and delete.
            move_GC_page(workingGCpage, newPage);
            log_logicalGC_add(logData, warmupset);
            if(reqNum == 0){
               GC_TAG = NOT_GC;
               GClevel = -1;
            }
         }
         else{
            write_into_new_page(newPage, new->hash);
            set_page_map(new->blkno / PAGE_SIZE, newPage, (char*)new->hash);
            write_into_hash_table(newPage, new->hash);
         }

         // full stripe part, update stripe state
         update_stripe_state(workingStripe[groupNo], newPage, TRACE_WRITE);
      }

      if(fullStripe[groupNo] == SSD_NUM){
         // actually we don't need to make much modification to new, the full
         // write part would do this automatically, doing resemble just want to
         // make sure gc pages is ok
         assemble_request(new, workingStripe[groupNo] ,writingPage);

         FULL_WRITE_TAG = 1;
         fullStripe[groupNo] = -1;
         writingPage = -1;
      }else{
         goto getRequest;
      }

      
      if(stripeIsFull(workingStripe[groupNo]) == 1){
         workingStripe[groupNo] = EMPTY_STATE;
         // when in gc state, we should not trigger another gc or it would be chaos
         if(enterGCstate() && GC_TAG == NOT_GC){
            GCstripe = find_gc_stripe();

            reqNum = GCpages;
            printf("now enter logical gc with stripe %d and %d pages for gc\n", GCstripe, GCpages);
            ASSERT(reqNum > 0);
            ASSERT(GClevel != EMPTY);
            GC_TAG = IN_GC;
         }
      }
   }
   
   if(logData->traceNum >= 1200000){
      return NULL;
   }

   if (new->flags & ASYNCHRONOUS) {
      new->flags |= (new->flags & READ) ? TIME_LIMITED : 0;
   } else if (new->flags & SYNCHRONOUS) {
      new->flags |= TIME_CRITICAL;
   }

   new->devno = 0;
   new->buf = 0;
   new->opid = 0;
   new->busno = 0;
   new->cause = 0;

   log_request_add(logData, new->flags, warmupset);

   //printf("blkno %d bcount %d hash %s", new->blkno, new->bcount, new->hash);
   return(new);
}

// other codes are negelected