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
#include "disksim_orgface.h"
#include "disksim_logorg.h"
#include "disksim_ioqueue.h"

#include "../ssdmodel/ssd_dedupRAID.h"

// other codes are negelected here

int logorg_parity_table(logorg *currlogorg, ioreq_event *curr, int numreqs)
{
   int i;
   ioreq_event *reqs[MAXDEVICES];
   ioreq_event *redunreqs[MAXDEVICES];
   int reqcnt = 0;
   ioreq_event *lastrow = NULL;
   int stripeunit;
   ioreq_event *temp;
   ioreq_event *newreq;
   int unitno;
   int stripeno;
   int entryno;
   int blkno;
   int blksinpart;
   int reqsize;
   int partsperstripe;
   int rowcnt = 1;
   int firstrow = 0;
   int opid = 0x1;
   int blkscovered;
   tableentry *table;
   int tablestart;
   int preventryno;
   /*
fprintf (outputfile, "Entered logorg_parity_table - devno %d, blkno %d, bcount %d, read %d\n", curr->devno, curr->blkno, curr->bcount, (curr->flags & READ));
*/
   if (numreqs != 1)
   {
      fprintf(stderr, "Multiple numreqs at logorg_parity_table is not acceptable - %d\n", numreqs);
      exit(1);
   }

   stripeunit = currlogorg->stripeunit;
   partsperstripe = currlogorg->partsperstripe;
   table = currlogorg->table;
   if (currlogorg->addrbyparts)
   {
      curr->blkno += curr->devno * currlogorg->blksperpart;
   }
   for (i = 0; i < currlogorg->actualnumdisks; i++)
   {
      reqs[i] = NULL;
   }
   unitno = curr->blkno / stripeunit;
   stripeno = unitno / partsperstripe;
   tablestart = (stripeno / currlogorg->tablestripes) * currlogorg->tablesize;
   stripeno = stripeno % currlogorg->tablestripes;
   blkno = tablestart + table[(stripeno * (partsperstripe + 1))].blkno;
   if (blkno == currlogorg->numfull)
   {
      stripeunit = currlogorg->actualblksperpart - blkno;
      curr->blkno -= blkno;
      unitno = curr->blkno / stripeunit;
   }
   blksinpart = stripeunit;
   unitno = unitno % partsperstripe;
   curr->blkno = curr->blkno % stripeunit;
   reqsize = curr->bcount;
   entryno = stripeno * partsperstripe + stripeno + unitno;
   blkno = tablestart + table[entryno].blkno;
   blksinpart -= curr->blkno;
   temp = ioreq_copy(curr);
   curr->next = temp;
   temp->blkno = blkno + curr->blkno;
   temp->devno = table[entryno].devno;
   temp->opid = 0;
   blkscovered = curr->bcount;
   curr->blkno = stripeno;
   curr->devno = unitno;
   curr->bcount = tablestart;
   reqs[temp->devno] = curr->next;
   temp->next = NULL;
   temp->prev = NULL;
   if (reqsize > blksinpart)
   {
      temp->bcount = blksinpart;
      blkscovered = blksinpart;
      reqsize -= blksinpart;
      unitno++;
      if (unitno == partsperstripe)
      {
         if (!(curr->flags & READ))
         {
            newreq = (ioreq_event *)getfromextraq();
            newreq->devno = table[(entryno + 1)].devno;
            newreq->blkno = table[(entryno + 1)].blkno;
            newreq->blkno += tablestart + temp->blkno - blkno;
            newreq->bcount = blkscovered;
            newreq->flags = curr->flags;
            newreq->opid = 0;
            reqs[newreq->devno] = newreq;
            newreq->next = NULL;
            temp->prev = newreq;
            newreq->prev = NULL;
         }
         blkscovered = 0;
         unitno = 0;
         if (firstrow == 0)
         {
            firstrow = 1;
         }
         rowcnt = 0;
         stripeno++;
         temp = NULL;
         if (stripeno == currlogorg->tablestripes)
         {
            stripeno = 0;
            tablestart += currlogorg->tablesize;
         }
      }
      entryno = (stripeno * partsperstripe) + stripeno + unitno;
      blkno = tablestart + table[entryno].blkno;
      blksinpart = (blkno != currlogorg->numfull) ? stripeunit : currlogorg->actualblksperpart - blkno;
      while (reqsize > blksinpart)
      {
         rowcnt++;
         newreq = (ioreq_event *)getfromextraq();
         newreq->blkno = blkno;
         newreq->devno = table[entryno].devno;
         newreq->bcount = blksinpart;
         blkscovered = max(blkscovered, blksinpart);
         newreq->flags = curr->flags;
         newreq->opid = 0;
         newreq->prev = NULL;
         if (temp)
         {
            temp->prev = newreq;
         }
         else
         {
            lastrow = newreq;
         }
         temp = newreq;
         logorg_parity_table_insert(&reqs[temp->devno], temp);
         reqsize -= blksinpart;
         unitno++;
         if (unitno == partsperstripe)
         {
            if (!(curr->flags & READ))
            {
               newreq = (ioreq_event *)getfromextraq();
               newreq->devno = table[(entryno + 1)].devno;
               newreq->blkno = table[(entryno + 1)].blkno + tablestart;
               newreq->bcount = blkscovered;
               newreq->flags = curr->flags;
               newreq->opid = 0;
               temp->prev = newreq;
               newreq->prev = NULL;
               logorg_parity_table_insert(&reqs[newreq->devno], newreq);
            }
            blkscovered = 0;
            unitno = 0;
            if (firstrow == 0)
            {
               firstrow = rowcnt;
            }
            rowcnt = 0;
            temp = NULL;
            stripeno++;
            if (stripeno == currlogorg->tablestripes)
            {
               stripeno = 0;
               tablestart += currlogorg->tablesize;
            }
         }
         entryno = (stripeno * partsperstripe) + stripeno + unitno;
         blkno = tablestart + table[entryno].blkno;
         blksinpart = (blkno != currlogorg->numfull) ? stripeunit : currlogorg->actualblksperpart - blkno;
      }
      newreq = (ioreq_event *)getfromextraq();
      newreq->blkno = blkno;
      newreq->devno = table[entryno].devno;
      newreq->bcount = reqsize;
      rowcnt++;
      blkscovered = max(blkscovered, blksinpart);
      newreq->flags = curr->flags;
      newreq->opid = 0;
      newreq->prev = NULL;
      if (temp)
      {
         temp->prev = newreq;
      }
      else
      {
         lastrow = newreq;
      }
      temp = newreq;
      logorg_parity_table_insert(&reqs[newreq->devno], newreq);
   }
   if (curr->flags & READ)
   {
      curr->next = curr;
      for (i = 0; i < currlogorg->actualnumdisks; i++)
      {
         reqcnt += logorg_join_seqreqs(reqs[i], curr, LOGORG_PARITY_SEQGIVE);
      }
   }
   else
   {
      preventryno = entryno;
      entryno = (stripeno * partsperstripe) + stripeno + partsperstripe;
      newreq = (ioreq_event *)getfromextraq();
      newreq->devno = table[entryno].devno;
      newreq->blkno = table[entryno].blkno + temp->blkno - table[preventryno].blkno;
      newreq->bcount = (rowcnt == 1) ? temp->bcount : blkscovered;
      newreq->flags = curr->flags;
      newreq->opid = 0;
      temp->prev = newreq;
      newreq->prev = NULL;
      logorg_parity_table_insert(&reqs[newreq->devno], newreq);
      if (firstrow == 0)
      {
         if ((rowcnt == 2) && ((curr->next->blkno - temp->blkno - temp->bcount) > 0))
         {
            newreq->bcount = temp->bcount;
            newreq = (ioreq_event *)getfromextraq();
            newreq->devno = temp->prev->devno;
            lastrow = temp;
            temp = curr->next;
            newreq->blkno = table[entryno].blkno + temp->blkno - table[(preventryno - 1)].blkno;
            newreq->bcount = temp->bcount;
            newreq->flags = curr->flags;
            newreq->opid = 0;
            temp->prev = newreq;
            newreq->prev = NULL;
            logorg_parity_table_insert(&reqs[newreq->devno], newreq);
            firstrow = 1;
            rowcnt = 1;
         }
         else
         {
            firstrow = rowcnt;
            rowcnt = 0;
         }
      }
      for (i = 0; i < currlogorg->actualnumdisks; i++)
      {
         redunreqs[i] = NULL;
      }
      if (firstrow < partsperstripe)
      {
         if (firstrow < currlogorg->rmwpoint)
         {
            logorg_parity_table_read_old(currlogorg, curr->next, redunreqs, opid);
         }
         else
         {
            logorg_parity_table_recon(currlogorg, curr->next, redunreqs, curr->blkno, curr->devno, curr->bcount, opid);
         }
         opid = opid << 1;
      }
      if ((rowcnt) && (rowcnt != partsperstripe))
      {
         if (rowcnt < currlogorg->rmwpoint)
         {
            logorg_parity_table_read_old(currlogorg, lastrow, redunreqs, opid);
         }
         else
         {
            logorg_parity_table_recon(currlogorg, lastrow, redunreqs, stripeno, 0, tablestart, opid);
         }
      }
      for (i = 0; i < currlogorg->actualnumdisks; i++)
      {
         curr->next = NULL;
         reqcnt += logorg_join_seqreqs(redunreqs[i], curr, LOGORG_PARITY_SEQGIVE);
         redunreqs[i] = curr->next;
         curr->next = NULL;
         reqcnt += logorg_join_seqreqs(reqs[i], curr, LOGORG_PARITY_SEQGIVE);
         reqs[i] = curr->next;
      }
      curr->next = curr;
      if (currlogorg->writesync)
      {
         logorg_parity_table_dodeps_sync(currlogorg, curr, redunreqs, reqs);
      }
      else
      {

         /* after hard work, I finially figured out how to modify
            this.

            this function does some damn dirty work to cut request, and wrap it
            into a request named reqs, and some shitty stuff called
            redunreqs. just format reqs, as my code shows. 

            Acatully I was planning to modify at an upper level, but it seems
            not working, I mean in disksim_iodriver.c:logorg_maprequest(), the
            result is passed here, as a list start from curr forming a loop.
            Fucking it worked initially but later failed miserably, so here I
            try to put it here.

            I used FULL_WRITE_TAG to indicate my modification here. if this is
            1, it means we are under my designed architecture.
         */
         if (FULL_WRITE_TAG == 1)
         {
            int dev = SSD_NUM;
            int blk = (page_to_stripe(BLKNO / PAGE_SIZE) * PAGES_PER_BLOCK +
                       page_to_stripe_align(BLKNO / PAGE_SIZE) % PAGES_PER_BLOCK) *
                      PAGE_SIZE;
            for (i = 0; i < SSD_NUM + 1; i++)
            {
               reqs[i] = ioreq_copy(curr);
               reqs[i]->blkno = blk;
               reqs[i]->bcount = PAGE_SIZE;
               reqs[i]->devno = dev;
               reqs[i]->flags = 0;
               reqs[i]->next = NULL;
               reqs[i]->opid = 0;
               
               redunreqs[i] = NULL;

               dev--;
            }
            for (i = 0; i < SSD_NUM; i++)
            {
               reqs[i]->prev = reqs[i + 1];
            }
            reqs[SSD_NUM]->prev = NULL;

            reqcnt = SSD_NUM + 1;
            FULL_WRITE_TAG = 0;
         }
         logorg_parity_table_dodeps_nosync(currlogorg, curr, redunreqs, reqs);
         // for(i=0; i<SSD_NUM+1;i++){
         //    printf("%p\n", redunreqs[i]);
         // }
      }
   }
   if (curr->next)
   {
      temp = curr->next;
      curr->blkno = temp->blkno;
      curr->devno = temp->devno;
      curr->bcount = temp->bcount;
      curr->flags = temp->flags;
      curr->next = temp->next;
      addtoextraq((event *)temp);
   }
   else
   {
      fprintf(stderr, "Seem to have no requests when leaving logorg_parity_table\n");
      exit(1);
   }
   /*
fprintf (outputfile, "Exiting logorg_parity_table - reqcnt %d\n", reqcnt);
*/
   return (reqcnt);
}

// other codes are negelcted here