C Copyright(C) 2011 National Technology & Engineering Solutions of
C Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
C NTESS, the U.S. Government retains certain rights in this software.
C 
C Redistribution and use in source and binary forms, with or without
C modification, are permitted provided that the following conditions are
C met:
C 
C * Redistributions of source code must retain the above copyright
C    notice, this list of conditions and the following disclaimer.
C           
C * Redistributions in binary form must reproduce the above
C   copyright notice, this list of conditions and the following
C   disclaimer in the documentation and/or other materials provided
C   with the distribution.
C                         
C * Neither the name of NTESS nor the names of its
C   contributors may be used to endorse or promote products derived
C   from this software without specific prior written permission.
C                                                 
C THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
C "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
C LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
C A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
C OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
C SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
C LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
C DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
C THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
C (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
C OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

C=======================================================================
      SUBROUTINE REFNOD (NELB, IDELB, NUMELB, NUMLNK, LINK, NUMNP, MAP)
C=======================================================================

C   --*** REFNOD *** (GREPOS) Reference nodes on all elements
C   --   Written by Greg Sjaardema - revised 03/02/90
C   --
C   --Parameters:
C   --   NELB - IN - the number of element blocks
C   --   IDELB - IN - the element block IDs for each block
C   --   NUMELB - IN - the number of elements in each block
C   --   NUMLNK - IN - the number of nodes per element in each block
C   --   LINK - IN - the connectivity for each block
C   --   NUMNP - IN/OUT - the number of nodes on input, the number of
C                         active nodes on exit.
C   --   MAP - OUT - list of active nodes
C   --               map(i) > 0 for active node
C   --               map(i) = length of map()+1 for deleted node

      INTEGER IDELB(NELB)
      INTEGER NUMELB(NELB)
      INTEGER NUMLNK(NELB)
      INTEGER LINK(*)
      INTEGER MAP(NUMNP)

      CALL INIINT (NUMNP, 0, MAP)

      IELNK = 0
      DO 10 IELB = 1, NELB
         ISLNK = IELNK + 1
         IELNK = IELNK + NUMLNK(IELB) * NUMELB(IELB)

         IF (IDELB(IELB) .NE. 0) THEN
            CALL REFND1 (NELB, NUMELB(IELB), NUMLNK(IELB), LINK(ISLNK),
     &         MAP)
         END IF
   10 CONTINUE

C .. Set up map of old number to new number.  If old id = I, then
C     new id = MAP(I)

      NEWID = 0
      DO 20 NP = 1, NUMNP
         IF (MAP(NP) .GT. 0) THEN
            NEWID = NEWID + 1
            MAP(NP) = NEWID
         ELSE
            MAP(NP) = NUMNP+1
         END IF
   20 CONTINUE
      NUMNP = NEWID

      RETURN
      END
