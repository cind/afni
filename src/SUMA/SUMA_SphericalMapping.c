#include "SUMA_suma.h"
#include "SUMA_Macros.h"
#if 0
   /* does not work on the MAC, check with Brenna about that inclusion */
   #include "malloc.h"
#endif 

float ep = 1e-4; /* this represents the smallest coordinate difference to be expected between neighboring nodes. Do not make it too small or else you will get round off errors. It is reassigned in SUMA_MakeIcosahedron, becoming dependent upon the recursion depth.  (Assigned here in case SUMA_binTesselate used without SUMA_CreateIcosahedron) Talk to Brenna Argall for details. */

/*!
   \brief face_nbad = SUMA_Bad_FacesetNorm_Dot_Radius(SO, FaceMask, dot_cut, face_bad_ind, face_bad_dot);
   find bad triangles in a sphere's mesh based on the dot product of the normal at that triangle and the radius
   \param SO (SUMA_SurfaceObject *) A surface Object (make sure faceset normals are current) 
   \param FaceMask (byte *) Optional mask for which triangles to analyze. If FaceMask[n] then 
                           triangle indexed n is analyzed.
                           Pass NULL to analyze all triangles
   \param dot_cut (double) dot products below dot_cut are flagged
   \param face_bad_ind (int *)   : if not null, it should hold up to SO->N_FaceSet elements and will contain,
                                    upon the function's return, the indices of triangles that had a dot product
                                    < dot_cut 
   \param face_bad_dot (float *) : if not null, it should hold up to SO->N_FaceSet elements and will contain,
                                    upon the function's return, the dot products of those troubled triangles.
   \param ReCalcNorm (int ): flag for recalculating triangle normals before proceeding. 
                           if 0 then SO->FaceNormList is used. Note that in either case, SO->FaceNormList
                           is unchanged.
   \return (int) The number of bad triangles encountered
*/
int SUMA_Bad_FacesetNorm_Dot_Radius(SUMA_SurfaceObject *SO, byte *FaceMask, double dot_cut, 
                                    int *face_bad_ind, float *face_bad_dot, int CalcNorm)
{
   static char FuncName[]={"SUMA_Bad_FacesetNorm_Dot_Radius"};
   int N_bad = -1, i, i3, n0, n0t, n1, n1t, n2, n2t;
   double dot, nr, r[3], cent[3], norm[3];
   float *P0, *P1, *P2;
   SUMA_Boolean LocalHead = NOPE;

   SUMA_ENTRY;

   if ((face_bad_ind && !face_bad_dot) || (!face_bad_ind && face_bad_dot)) {
      SUMA_S_Err("Both of face_bad_ind and face_bad_dot must be either NULL or valid pointers");
      SUMA_RETURN(N_bad);
   }
   
   /*
   if (CalcNorm) { fprintf(stderr,"CalcNorm = 1\n"); } 
   else { fprintf(stderr,"CalcNorm = 0\n"); };
   */
   
   N_bad = 0;
   for (i=0; i < SO->N_FaceSet; ++i) {
      if (!FaceMask || (FaceMask && FaceMask[i])) {
         i3 = 3*i;
         n0 = SO->FaceSetList[i3  ]; n0t = 3* n0;
         n1 = SO->FaceSetList[i3+1]; n1t = 3* n1;
         n2 = SO->FaceSetList[i3+2]; n2t = 3* n2;

         /* Calculate Center of Gravity of each facet. */
         cent[0  ] = (  SO->NodeList[n0t  ] + 
                        SO->NodeList[n1t  ] + 
                        SO->NodeList[n2t  ] )  / 3.0;
         cent[1  ] = (  SO->NodeList[n0t+1] + 
                        SO->NodeList[n1t+1] + 
                        SO->NodeList[n2t+1] )  / 3.0;
         cent[2  ] = (  SO->NodeList[n0t+2] + 
                        SO->NodeList[n1t+2] + 
                        SO->NodeList[n2t+2] )  / 3.0;

         /* calculate radius vector */
         r[0] = cent[0  ] - SO->Center[0];
         r[1] = cent[1  ] - SO->Center[1];
         r[2] = cent[2  ] - SO->Center[2];

         /* scale radius vector */
         nr = sqrt ( r[0] * r[0] + r[1] * r[1] + r[2] * r[2] );
         r[0] /= nr; r[1] /= nr; r[2] /= nr; 

         if (!CalcNorm) {
         dot = r[0]*SO->FaceNormList[i3  ] + 
               r[1]*SO->FaceNormList[i3+1] +
               r[2]*SO->FaceNormList[i3+2] ;
         } else {
            P0 = &(SO->NodeList[n0t  ]);
            P1 = &(SO->NodeList[n1t  ]);
            P2 = &(SO->NodeList[n2t  ]);
            SUMA_TRI_NORM_NORM(P0, P1, P2, norm);
            dot = r[0]*norm[0] + r[1]*norm[1] + r[2]*norm[2];
         }
         if (dot < dot_cut) {
            if (face_bad_ind) {
               face_bad_ind[N_bad] = i;  
               face_bad_dot[N_bad] = (float) dot;
            }
            ++N_bad;
         }
      }
   }
   SUMA_RETURN(N_bad);
}


/*!
   \brief A function to test if a spherical surface is indeed spherical
   
   
   This function reports on a few parameters indicative of
   the quality of a spherical surface:
   it calculates the absolute deviation between
   the distance of each node from SO->Center (d) and the estimated radius(r)
      abs (d - r) 
   The distances are  written to the file: <Froot>_Ddist.1D . The
   first column represents node index. A colorized version is written to 
   <Froot>_Ddist.1D.col (node index followed by r g b values)
   
   The function also computes the cosine of the angle between the normal at
   a node and the direction vector formed by the center and that node. 
   Since both vectors are normalized, the cosine of the angle is the dot product. 
   On a sphere, the abs(dot product) should be 1 or pretty close. abs(dot product) < 0.9 are 
   flagged as bad and written out to the file <Froot>_BadNodes.1D . 
   The file <Froot>_dotprod.1D contains the dot product values for all the 
   nodes. The file with colorized results are <Froot>_BadNodes.1D.col and 
   <Froot>_dotprod.1D.col
      
*/
SUMA_Boolean SUMA_SphereQuality(SUMA_SurfaceObject *SO, 
                        char *Froot, char *shist, 
                        int *N_bad_nodesu, int *N_bad_facesetsu)
{
   static char FuncName[]={"SUMA_SphereQuality"};
   float *dist = NULL, mdist, *dot=NULL, nr, r[3], *bad_dot = NULL;
   float *face_dot=NULL, *face_bad_dot = NULL, *face_cent = NULL;
   float dmin, dmax, dminloc, dmaxloc;
   int i, i3, *isortdist = NULL;
   int *bad_ind = NULL, ibad =-1;
   int *face_bad_ind = NULL, face_ibad =-1;
   int F[3];
   FILE *fid;
   FILE *face_id;
   char *fname;
   float dot_cut = 0.00001;
   double cent[3]={0.0, 0.0, 0.0};
   double centmed[3]={0.0, 0.0, 0.0};
   SUMA_COLOR_MAP *CM;
   SUMA_SCALE_TO_MAP_OPT * OptScl;
   SUMA_COLOR_SCALED_VECT * SV;
   SUMA_Boolean LocalHead = NOPE;
   
   SUMA_ENTRY;
   
   if (N_bad_nodesu) *N_bad_nodesu = -1;
   if (N_bad_facesetsu) *N_bad_facesetsu = -1;
   
   if (!SO) {
      SUMA_SL_Err("NULL SO");
      SUMA_RETURN(0);
   }
   
   /* get the options for creating the scaled color mapping */
   OptScl = SUMA_ScaleToMapOptInit();
   if (!OptScl) {
      fprintf (SUMA_STDERR,
               "Error %s: Could not get scaling option structure.\n", FuncName);
      exit (1); 
   }
   
   /* get the color map */
   CM = SUMA_FindNamedColMap ("byr64");
   if (CM == NULL) {
      fprintf (SUMA_STDERR,
               "Error %s: Could not get standard colormap.\n", FuncName);
      if (OptScl) SUMA_free(OptScl);
      exit (1); 
   }
   
   if (!SUMA_GetCenterOfSphereSurface(SO, 500, cent, centmed)) {
      SUMA_S_Err("Failed to get center");
   }else{
      SUMA_S_Notev("Center of mass of surface is:\n"
                  "  [%f   %f   %f]\n"
                  "Estimated center of surface is:\n"
                  "  [%f   %f   %f]\n"
                  "Median estimated center of surface is:\n"
                  "  [%f   %f   %f]\n",
                  SO->Center[0], SO->Center[1], SO->Center[2],
                  cent[0], cent[1], cent[2],
                  centmed[0], centmed[1], centmed[2]);
   }
   

   /* compare the distance of each node to the distance to estimated radius */
   dist = (float *)SUMA_calloc(SO->N_Node, sizeof(float));
   mdist = 0.0;
   for (i=0; i<SO->N_Node; ++i) {
      i3 = 3*i;
      dist[i] =   sqrt ( pow((double)(SO->NodeList[i3]   - centmed[0]), 2.0) +
                         pow((double)(SO->NodeList[i3+1] - centmed[1]), 2.0) +
                         pow((double)(SO->NodeList[i3+2] - centmed[2]), 2.0) );
      mdist += dist[i];
   }
   mdist /= (float)SO->N_Node;
   
   /* calculate the difference from mdist */
   for (i=0; i<SO->N_Node; ++i) dist[i] = fabs(dist[i] - mdist);
   
   
   /* Colorize results */
   SV = SUMA_Create_ColorScaledVect(SO->N_Node, 0);
   if (!SV) {
      fprintf (SUMA_STDERR,"Error %s: Could not allocate for SV.\n", FuncName);
      if (dist) SUMA_free(dist);
      if (OptScl) SUMA_free(OptScl);
      exit(1);
   }
   SUMA_MIN_MAX_VEC(dist, SO->N_Node, dmin, dmax, dminloc, dmaxloc);
   if (!SUMA_ScaleToMap (dist, SO->N_Node, dmin, dmax, CM, OptScl, SV)) {
      fprintf (SUMA_STDERR,"Error %s: Failed in SUMA_ScaleToMap.\n", FuncName);
      if (dist) SUMA_free(dist);
      if (OptScl) SUMA_free(OptScl);
      exit(1);
   }

   /* write the data */
   fname = SUMA_append_string(Froot, "_Dist.1D.dset");
   if (LocalHead) fprintf (SUMA_STDERR,"%s:\nWriting %s...\n", FuncName, fname);
   fid = fopen(fname, "w");
   fprintf(fid,"#Absolute difference between node distance from estimated geometric center of %f %f %f, and average radius.\n"
               "#col 0: Node Index\n"
               "#col 1: distance\n", centmed[0], centmed[1], centmed[2]);
   if (shist) fprintf(fid,"#History:%s\n", shist);
   for (i=0; i<SO->N_Node; ++i) fprintf(fid,"%d\t%f\n", i, dist[i]);
   fclose(fid);
   SUMA_free(fname); fname = NULL;
 
   /* write the colorized data */
   fname = SUMA_append_string(Froot, "_Dist.1D.col");
   if (LocalHead) fprintf (SUMA_STDERR,"%s:\nWriting %s...\n", FuncName, fname);
   fid = fopen(fname, "w");
   fprintf(fid,"#Color file of absolute difference between node distance from estimated geometric center of %f %f %f,  and average radius.\n"
               "#col 0: Node Index\n"
               "#col 1: R\n"
               "#col 2: G\n"
               "#col 3: B\n", centmed[0], centmed[1], centmed[2]);
   if (shist) fprintf(fid,"#History:%s\n", shist);
   for (i=0; i<SO->N_Node; ++i) fprintf(fid,"%d\t%f\t%f\t%f\n", i, SV->cV[3*i  ], SV->cV[3*i+1], SV->cV[3*i+2]);
   fclose(fid);
   SUMA_free(fname); fname = NULL;
   if (SV) SUMA_Free_ColorScaledVect (SV);
   
   /* Now sort that */ 
   isortdist = SUMA_z_qsort ( dist , SO->N_Node  );
   
   /* report */
   fprintf (SUMA_STDERR,"\n");
   fprintf (SUMA_STDERR,"%s: \n"
                        "Reporting on Spheriosity of %s\n", FuncName, SO->Label);
   fprintf (SUMA_STDERR," Mean distance from geometric center (estimated radius): %f\n", mdist);
   fprintf (SUMA_STDERR," Largest 10 absolute departures from estimated radius:\n"
                        " See output files for more detail.\n");
   for (i=SO->N_Node-1; i > SO->N_Node - 10; --i) {
      fprintf (SUMA_STDERR,"dist @ %d: %f\n", isortdist[i], dist[i]); 
   }
   
   /* write the FaceSetList to file */
   fname = SUMA_append_string(Froot, "_FaceSetList.1D.dset");
   if (LocalHead) fprintf (SUMA_STDERR,"%s:\nWriting %s...\n", FuncName, fname);
   fid = fopen(fname, "w");
   fprintf(fid,"#FaceSetList.\n"
               "#col 0: Facet Index\n");
   if (shist) fprintf(fid,"#History:%s\n\n", shist);
   for (i=0; i<SO->N_FaceSet; ++i) { 
      i3 = 3*i; 
      fprintf(fid,"%d   %d    %d    %d\n", 
                  i, SO->FaceSetList[i3  ], SO->FaceSetList[i3+1], SO->FaceSetList[i3+2]);
   }
   fclose(fid);
   SUMA_free(fname); fname = NULL;
      
   /* write the FaceNormList to file */
   fname = SUMA_append_string(Froot, "_FaceNormList.1D.dset");
   if (LocalHead) fprintf (SUMA_STDERR,"%s:\nWriting %s...\n", FuncName, fname);
   fid = fopen(fname, "w");
   fprintf(fid,"#Facet Normals.\n"
               "#col 0: Facet Index\n\n");
   if (shist) fprintf(fid,"#History:%s\n", shist);
   for (i=0; i<SO->N_FaceSet; ++i) { 
      i3 = 3*i; 
      fprintf(fid,"%d   %f    %f    %f\n", 
                  i, SO->FaceNormList[i3  ], SO->FaceNormList[i3+1], SO->FaceNormList[i3+2]);
   }
   fclose(fid);
   SUMA_free(fname); fname = NULL;
   
   /* 
   If we had a perfect sphere then the normal of each node
   will be colinear with the direction of the vector between the
   sphere's center and the node.
   The more the deviation, the worse the sphere */
   dot     = (float *)SUMA_calloc(SO->N_Node, sizeof(float));
   bad_ind = (int *)  SUMA_calloc(SO->N_Node, sizeof(int)  );
   bad_dot = (float *)SUMA_calloc(SO->N_Node, sizeof(float));
   ibad = 0;
   for (i=0; i<SO->N_Node; ++i) {
      i3 = 3*i;
      r[0] = SO->NodeList[i3]   - centmed[0];
      r[1] = SO->NodeList[i3+1] - centmed[1];
      r[2] = SO->NodeList[i3+2] - centmed[2];
      nr = sqrt ( r[0] * r[0] + r[1] * r[1] + r[2] * r[2] );
      r[0] /= nr; r[1] /= nr; r[2] /= nr; 
      
      dot[i] = r[0]*SO->NodeNormList[i3]   + 
               r[1]*SO->NodeNormList[i3+1] +
               r[2]*SO->NodeNormList[i3+2] ;
      
      if (fabs(dot[i]) < 0.9) {
         bad_ind[ibad] = i;
         bad_dot[ibad] = dot[i];
         ++ibad;
      }
   }
   
   bad_ind = (int *)  SUMA_realloc(bad_ind, ibad * sizeof(int));
   bad_dot = (float *)SUMA_realloc(bad_dot, ibad * sizeof(float));
   
      fname = SUMA_append_string(Froot, "_dotprod.1D.dset");
      if (LocalHead) 
         fprintf (SUMA_STDERR,"%s:\nWriting %s...\n", FuncName, fname);
      fid = fopen(fname, "w");
      fprintf(fid,"#Cosine of node normal angles with radial direction\n"
                  "#col 0: Node Index\n"
                  "#col 1: cos(angle)\n"
                  ); 
      if (shist) fprintf(fid,"#History:%s\n", shist);
      for (i=0; i<SO->N_Node; ++i) fprintf(fid,"%d\t%f\n", i, dot[i]);
      fclose(fid);
      SUMA_free(fname); fname = NULL;
      
      /* write the colorized data */
      SV = SUMA_Create_ColorScaledVect(SO->N_Node, 0);
      if (!SV) {
         fprintf (SUMA_STDERR,
                  "Error %s: Could not allocate for SV.\n", FuncName);
         if (dot) SUMA_free(dot);
         if (bad_dot) SUMA_free(bad_dot);
         if (bad_ind) SUMA_free(bad_ind);
         if (isortdist) SUMA_free(isortdist);
         if (dist) SUMA_free(dist);
         if (OptScl) SUMA_free(OptScl);
         exit(1);
      }

      if (!SUMA_ScaleToMap (dot, SO->N_Node, -1.0, 1.0, CM, OptScl, SV)) {
         fprintf (SUMA_STDERR,
                  "Error %s: Failed in SUMA_ScaleToMap.\n", FuncName);
         exit(1);
      }
      fname = SUMA_append_string(Froot, "_dotprod.1D.col");
      if (LocalHead) 
         fprintf (SUMA_STDERR,"%s:\nWriting %s...\n", FuncName, fname);
      fid = fopen(fname, "w");
      fprintf(fid,
         "#Color file of cosine of node normal angles with radial direction\n"
         "#col 0: Node Index\n"
         "#col 1: R\n"
         "#col 2: G\n"
         "#col 3: B\n"
                  ); 
      if (shist) fprintf(fid,"#History:%s\n", shist);
      for (i=0; i<SO->N_Node; ++i) 
         fprintf(fid,"%d\t%f\t%f\t%f\n", 
                     i, SV->cV[3*i  ], SV->cV[3*i+1], SV->cV[3*i+2]);
      fclose(fid);
      SUMA_free(fname); fname = NULL;
      if (SV) SUMA_Free_ColorScaledVect (SV);
      
      fname = SUMA_append_string(Froot, "_BadNodes.1D.dset");
      if (LocalHead) 
         fprintf (SUMA_STDERR,"%s:\nWriting %s...\n", FuncName, fname);
      fid = fopen(fname, "w");
      fprintf(fid,
   "#Nodes with normals at angle with radial direction: abs(dot product < 0.9)\n"
   "#col 0: Node Index\n"
   "#col 1: cos(angle)\n"
               ); 
      if (shist) fprintf(fid,"#History:%s\n", shist);
      for (i=0; i<ibad; ++i) fprintf(fid,"%d\t%f\n", bad_ind[i], bad_dot[i]);
      fclose(fid);
      SUMA_free(fname); fname = NULL;
      
      /* write the colorized data */
      SV = SUMA_Create_ColorScaledVect(ibad, 0);
      if (!SV) {
         fprintf (SUMA_STDERR,
                  "Error %s: Could not allocate for SV.\n", FuncName);
         if (dot) SUMA_free(dot);
         if (bad_dot) SUMA_free(bad_dot);
         if (bad_ind) SUMA_free(bad_ind);
         if (isortdist) SUMA_free(isortdist);
         if (dist) SUMA_free(dist);
         if (OptScl) SUMA_free(OptScl);
         exit(1);
      }

      if (!SUMA_ScaleToMap (bad_dot, ibad, -1.0, 1.0, CM, OptScl, SV)) {
         fprintf (SUMA_STDERR,
                  "Error %s: Failed in SUMA_ScaleToMap.\n", FuncName);
         if (dot) SUMA_free(dot);
         if (bad_dot) SUMA_free(bad_dot);
         if (bad_ind) SUMA_free(bad_ind);
         if (isortdist) SUMA_free(isortdist);
         if (dist) SUMA_free(dist);
         if (OptScl) SUMA_free(OptScl);
         exit(1);
      }
      fname = SUMA_append_string(Froot, "_BadNodes.1D.col");
      if (LocalHead) fprintf (SUMA_STDERR,"%s:\nWriting %s...\n", FuncName, fname);
      fid = fopen(fname, "w");
      fprintf(fid,"#Color file of nodes with normals at angle with radial direction: abs(dot product < 0.9)\n"
                  "#col 0: Node Index\n"
                  "#col 1: R\n"
                  "#col 2: G\n"
                  "#col 3: B\n" ); 
      if (shist) fprintf(fid,"#History:%s\n", shist);
      for (i=0; i<ibad; ++i) fprintf(fid,"%d\t%f\t%f\t%f\n", bad_ind[i], SV->cV[3*i  ], SV->cV[3*i+1], SV->cV[3*i+2]);
      fclose(fid);
      SUMA_free(fname); fname = NULL;
      if (SV) SUMA_Free_ColorScaledVect (SV);
      
   
   /* report, just 10 of them  */
   {
      int nrep;
      nrep = SUMA_MIN_PAIR(ibad, 10); 
      fprintf (SUMA_STDERR,"%d of the %d nodes with normals at angle with radial direction\n"
                           " i.e. abs(dot product < 0.9)\n"
                           " See output files for full list\n", nrep, ibad);
      for (i=0; i < nrep; ++i) {
         fprintf (SUMA_STDERR,"cos(ang) @ node %d: %f\n", bad_ind[i], bad_dot[i]);
      } 
   }  
 
   /*
   Compare the normal of each facet to the direction of the vector
   between the sphere's center and the center of the facet.  
   Use the center of mass of the triangle as the center of the facet.
   If we had a perfect sphere then these vectors would
   be colinear. The more the deviation, the worse the sphere */
   
   face_cent      = (float *)SUMA_calloc(3*SO->N_FaceSet, sizeof(float));
   face_dot       = (float *)SUMA_calloc(SO->N_FaceSet, sizeof(float));
   face_bad_ind   = (int *)  SUMA_calloc(SO->N_FaceSet, sizeof(int)  );
   face_bad_dot   = (float *)SUMA_calloc(SO->N_FaceSet, sizeof(float));

   face_ibad = SUMA_Bad_FacesetNorm_Dot_Radius(SO, NULL, 0.0001, face_bad_ind, face_bad_dot, 0);
       
   /* write the data */
   fname = SUMA_append_string(Froot, "_facedotprod.1D.dset");
   if (LocalHead) fprintf (SUMA_STDERR,"%s:\nWriting %s...\n", FuncName, fname);
   face_id= fopen(fname, "w");
   fprintf(face_id,"#Cosine of facet normal angles with radial direction from facet center\n"
               "#col 0: Facet Index\n"
               "#col 1: cos(angle)\n"
               ); 
   if (shist) fprintf(face_id,"#History:%s\n", shist);
   for (i=0; i<SO->N_FaceSet; ++i) fprintf(face_id,"%d\t%f\n", i, face_dot[i]);
   fclose(face_id);
   SUMA_free(fname); fname = NULL;

   fname = SUMA_append_string(Froot, "_BadFaceSets.1D.dset");
   if (LocalHead) fprintf (SUMA_STDERR,"%s:\nWriting %s...\n", FuncName, fname);
   face_id= fopen(fname, "w");
   fprintf(face_id,"#Facets with normals at angle with radial direction: (dot product < %f)\n"
               "#col 0: Facet Index\n"
               "#col 1: cos(angle)\n"
               , dot_cut); 
   if (shist) fprintf(face_id,"#History:%s\n", shist);
   for (i=0; i<face_ibad; ++i) fprintf(face_id,"%d\t%f\n", face_bad_ind[i], face_bad_dot[i]);
   fclose(face_id);
   SUMA_free(fname); fname = NULL;

   /* report, just 10 of them  */
   {
      int face_nrep;
      face_nrep = SUMA_MIN_PAIR(face_ibad, 10); 
      fprintf (SUMA_STDERR,"%d of the %d facets with normals at angle with radial direction\n"
                           " i.e. (dot product < %f)\n"
                           " See output files for full list\n", face_nrep, face_ibad, dot_cut);
      for (i=0; i < face_nrep; ++i) {
         fprintf (SUMA_STDERR,"cos(ang) @ facet %d: %f\n", face_bad_ind[i], face_bad_dot[i]);
      /* If face_nrep is zero, then this will not be printed. */
      } 
   }
     
   if (dot) SUMA_free(dot);
   if (bad_dot) SUMA_free(bad_dot);
   if (bad_ind) SUMA_free(bad_ind);
   if (face_cent) SUMA_free(face_cent);
   if (face_dot) SUMA_free(face_dot);
   if (face_bad_dot) SUMA_free(face_bad_dot);
   if (face_bad_ind) SUMA_free(face_bad_ind); 
   if (isortdist) SUMA_free(isortdist);
   if (dist) SUMA_free(dist);
   if (OptScl) SUMA_free(OptScl);

/* CAREFUL, MIGHT HAVE CHANGED RETURN VARIABLE TO REFLECT FACET DEVIATIONS INSTEAD OF BAD NODES.  
      Use "(face_ibad)" if want to flag in program when first bad facet occurs.
      Otherwise, use original return variable.  Before was just "(ibad)" */  
   
   if (N_bad_nodesu) *N_bad_nodesu = ibad;
   if (N_bad_facesetsu) *N_bad_facesetsu = face_ibad;
   
   SUMA_RETURN(YUP);
}

SUMA_SPHERE_QUALITY *SUMA_Free_SphereQuality(SUMA_SPHERE_QUALITY *SSQ)
{
   static char FuncName[]={"SUMA_Free_SphereQuality"};
   SUMA_ENTRY;
   if (!SSQ) SUMA_RETURN(NULL);
   if (SSQ->node_DelDist) SUMA_free(SSQ->node_DelDist);
   if (SSQ->node_DelDot) SUMA_free(SSQ->node_DelDot);
   if (SSQ->face_DelDot) SUMA_free(SSQ->face_DelDot);
   if (SSQ->node_Conv) SUMA_free(SSQ->node_Conv);
   SUMA_free(SSQ); 
   SUMA_RETURN(NULL);
}

SUMA_SPHERE_QUALITY *SUMA_Alloc_SphereQuality(int N, int F)
{
   static char FuncName[]={"SUMA_Alloc_SphereQuality"};
   SUMA_SPHERE_QUALITY *SSQ = NULL;
   
   SUMA_ENTRY;
   
   SSQ = (SUMA_SPHERE_QUALITY *)SUMA_calloc(1,sizeof(SUMA_SPHERE_QUALITY)); 
   
   SSQ->N_bad_nodes = -1;
   SSQ->N_bad_facesets = -1;
   SSQ->N_node = N;
   SSQ->N_face = F;
   
   SSQ->node_DelDist = (float*)SUMA_calloc(N,sizeof(float)); 
   SSQ->node_DelDot = (float*)SUMA_calloc(N,sizeof(float)); 
   SSQ->node_Conv = (float*)SUMA_calloc(N,sizeof(float)); 
   SSQ->face_DelDot = (float*)SUMA_calloc(F,sizeof(float)); 
   
   SSQ->cm[0] = SSQ->cm[1] = SSQ->cm[2] = 0.0;
   SSQ->AvgDist = 0.0;
   
   SUMA_RETURN(SSQ);
}

SUMA_SPHERE_QUALITY *
   SUMA_SphericalDeviations(SUMA_SurfaceObject *SO, 
                            SUMA_SPHERE_QUALITY *SSQ, char *opts)
{
   static char FuncName[]={"SUMA_SphericalDeviations"};
   float nr, r[3], *n1, *n2, *n3;
   int i, i3;
   double cent[3]={0.0, 0.0, 0.0};
   double centmed[3]={0.0, 0.0, 0.0};
   SUMA_Boolean LocalHead = NOPE;
   
   SUMA_ENTRY;
   
   if (SSQ) { 
      SSQ->N_bad_nodes = -1;
      SSQ->N_bad_facesets = -1;
   }
   if (!opts) opts = "dist;dot";
   
   if (!SO) {
      SUMA_SL_Err("NULL SO");
      SUMA_RETURN(SSQ);
   }
   if (!SSQ || SSQ->N_node != SO->N_Node || SSQ->N_face != SO->N_FaceSet) {
      SUMA_Free_SphereQuality(SSQ);
      SSQ = SUMA_Alloc_SphereQuality(SO->N_Node, SO->N_FaceSet);
   }
   
       
   if (!SUMA_GetCenterOfSphereSurface(SO, 500, cent, centmed)) {
      SUMA_S_Err("Failed to get center");
      SUMA_RETURN(SSQ);
   }else{
      SUMA_LHv("Center of mass of surface is:\n"
                  "  [%f   %f   %f]\n"
                  "Estimated center of surface is:\n"
                  "  [%f   %f   %f]\n"
                  "Median estimated center of surface is:\n"
                  "  [%f   %f   %f]\n",
                  SO->Center[0], SO->Center[1], SO->Center[2],
                  cent[0], cent[1], cent[2],
                  centmed[0], centmed[1], centmed[2]);
   }
   SSQ->cm[0] = centmed[0]; SSQ->cm[1] = centmed[1]; SSQ->cm[2] = centmed[2];


   /* compare the distance of each node to the distance to estimated radius */
   SSQ->AvgDist = 0.0;
   for (i=0; i<SO->N_Node; ++i) {
      i3 = 3*i;
      SSQ->node_DelDist[i] =   
                  sqrt ( pow((double)(SO->NodeList[i3]   - centmed[0]), 2.0) +
                         pow((double)(SO->NodeList[i3+1] - centmed[1]), 2.0) +
                         pow((double)(SO->NodeList[i3+2] - centmed[2]), 2.0) );
      SSQ->AvgDist += SSQ->node_DelDist[i];
   }
   SSQ->AvgDist /= (float)SO->N_Node;
   
   /* calculate the difference from mdist */
   for (i=0; i<SO->N_Node; ++i) 
      SSQ->node_DelDist[i] = fabs(SSQ->node_DelDist[i] - SSQ->AvgDist);
   
   /* calculate 1-abs(dot) for angle between radius and node normal*/ 
   SSQ->N_bad_nodes = 0;
   for (i=0; i<SO->N_Node; ++i) {
      i3 = 3*i;
      r[0] = SO->NodeList[i3]   - centmed[0];
      r[1] = SO->NodeList[i3+1] - centmed[1];
      r[2] = SO->NodeList[i3+2] - centmed[2];
      nr = sqrt ( r[0] * r[0] + r[1] * r[1] + r[2] * r[2] );
      r[0] /= nr; r[1] /= nr; r[2] /= nr; 
      
      SSQ->node_DelDot[i] = 
               r[0]*SO->NodeNormList[i3]   + 
               r[1]*SO->NodeNormList[i3+1] +
               r[2]*SO->NodeNormList[i3+2] ;
      
      SSQ->node_DelDot[i] = 1.0 - fabs(SSQ->node_DelDot[i]);
      if (SSQ->node_DelDot[i]>0.1) ++SSQ->N_bad_nodes;
   }
      
   
   /* calculate 1-abs(dot) for angle between faceset nornal and radius */
   
   SSQ->N_bad_facesets = 0;
   for (i=0; i<SO->N_FaceSet; ++i) {
      i3 = 3*i;
      n1 = SO->NodeList+3*SO->FaceSetList[3*i  ];
      n2 = SO->NodeList+3*SO->FaceSetList[3*i+1];
      n3 = SO->NodeList+3*SO->FaceSetList[3*i+2];
      r[0] = (n1[0]+n2[0]+n3[0])/3.0 - centmed[0];
      r[1] = (n1[1]+n2[1]+n3[1])/3.0 - centmed[1];
      r[2] = (n1[2]+n2[2]+n3[2])/3.0 - centmed[2];
      nr = sqrt ( r[0] * r[0] + r[1] * r[1] + r[2] * r[2] );
      r[0] /= nr; r[1] /= nr; r[2] /= nr; 
      
      SSQ->face_DelDot[i] = 
               r[0]*SO->FaceNormList[i3]   + 
               r[1]*SO->FaceNormList[i3+1] +
               r[2]*SO->FaceNormList[i3+2] ;
      
      SSQ->face_DelDot[i] = 1.0 - fabs(SSQ->face_DelDot[i]);
      if (SSQ->face_DelDot[i]>0.1) ++SSQ->N_bad_facesets;
   }

   /* calculate convexity */
   if (!opts || strstr(opts,"conv")) {
      SUMA_Convexity( SO->NodeList, SO->N_Node, 
                      SO->NodeNormList, SO->FN, SSQ->node_Conv);
   }
   
   SUMA_RETURN(SSQ);
}

/*!
  SUMA_binTesselate(nodeList, triList, nCtr, tCtr, recDepth, depth, n1, n2, n3);

  This function divides 1 triangle into 4 recursively to depth recDepth.
  \param nodeList (float *) 3 x N_Node list of nodes (updated as new nodes created during tessellation)
  \param triList (int *) 3 x N_Triangle list of nodes associated with each triangle (updated as new triangles created during tessellation)
  \param nCtr (int *) index of most recently added node to nodeList
  \param tCtr (int *) index of most recently added triangle to triList
  \param recDepth (int) recursion depth
  \param depth (int) current depth
  \param n1, n2, n3 (int) indices in nodeList corresponding to three nodes of triangle being tessellated
  \return void (but nodeList and triList updated)

  Written by Brenna Argall
 
*/

void SUMA_binTesselate(float *nodeList, int *triList, int *nCtr, int *tCtr, int recDepth, int depth, int n1, int n2, int n3)
{
   double x1=0,y1=0,z1=0, x2=0,y2=0,z2=0, x3=0,y3=0,z3=0;
   double x12=0, y12=0, z12=0;
   double x23=0, y23=0, z23=0;
   double x31=0, y31=0, z31=0;
   int currIndex, index1, index2, index3;
   int i=0, j=0, m=0, k=0;
   static char FuncName[]={"SUMA_binTesselate"};
   
   SUMA_ENTRY;

   currIndex = (nCtr[0]-2)/3;

   x1=(double)nodeList[3*n1]; y1=(double)nodeList[3*n1+1]; z1=(double)nodeList[3*n1+2];
   x2=(double)nodeList[3*n2]; y2=(double)nodeList[3*n2+1]; z2=(double)nodeList[3*n2+2];
   x3=(double)nodeList[3*n3]; y3=(double)nodeList[3*n3+1]; z3=(double)nodeList[3*n3+2];
  
   x12=(x1+x2)/2.0; y12=(y1+y2)/2.0; z12=(z1+z2)/2.0;
   x23=(x2+x3)/2.0; y23=(y2+y3)/2.0; z23=(z2+z3)/2.0;
   x31=(x3+x1)/2.0; y31=(y3+y1)/2.0; z31=(z3+z1)/2.0;

   /**prevents creation of duplicate nodes*/
   index1 = -1; index2 = -1; index3 = -1;
   i=0; j=0;
   for (i=0; i<=currIndex; ++i) {
      j = 3*i;
      if ( fabs(nodeList[j]-x12)<ep && fabs(nodeList[j+1]-y12)<ep && fabs(nodeList[j+2]-z12)<ep ) {
         index1 = i;
      }
      if ( fabs(nodeList[j]-x23)<ep && fabs(nodeList[j+1]-y23)<ep && fabs(nodeList[j+2]-z23)<ep ) {
         index2 = i;
      }
      if ( fabs(nodeList[j]-x31)<ep && fabs(nodeList[j+1]-y31)<ep && fabs(nodeList[j+2]-z31)<ep ) {
         index3 = i;
      }
   }
  
   if (index1==-1) {
      ++currIndex;
      index1 = currIndex;
      SUMA_addNode( nodeList, nCtr, (float)x12, (float)y12, (float)z12);
   }
   if (index2==-1) {
      ++currIndex;
      index2 = currIndex;
      SUMA_addNode( nodeList, nCtr, (float)x23, (float)y23, (float)z23);
   }
   if (index3==-1) {
      ++currIndex;
      index3 = currIndex;
      SUMA_addNode( nodeList, nCtr, (float)x31, (float)y31, (float)z31);
   }
  
   /**if recursion depth met, add 4 triangles to list referenced by tPtr*/
   if (depth>=recDepth) {
      SUMA_addTri( triList, tCtr, n1, index1, index3);
      SUMA_addTri( triList, tCtr, index1, n2, index2);
      SUMA_addTri( triList, tCtr, index3, index2, n3);
      SUMA_addTri( triList, tCtr, index3, index2, index1);
   }

   /**recursion depth not met: call tessellate on each of 4 new triangles*/
   else {
      ++depth;
      SUMA_binTesselate( nodeList, triList, nCtr, tCtr, recDepth, 
                        depth, n1, index1, index3 );
      SUMA_binTesselate( nodeList, triList, nCtr, tCtr, recDepth, 
                        depth, index1, n2, index2 );
      SUMA_binTesselate( nodeList, triList, nCtr, tCtr, recDepth, 
                        depth, index3, index2, n3 );
      SUMA_binTesselate( nodeList, triList, nCtr, tCtr, recDepth, 
                        depth, index3, index2, index1 );
   }

   SUMA_RETURNe;
}

/*!
  SUMA_tesselate(nodeList, triList, nCtr, tCtr, N_Div, n0, n1, n2);

  This function tesselates triangle by dividing edges into N_Div segments.
  \param nodeList (float *) 3 x N_Node list of nodes (updated as new nodes created during tessellation)
  \param triList (int *) 3 x N_Triangle list of nodes associated with each triangle (updated as new triangles created during tessellation)
  \param nCtr (int *) index of most recently added node to nodeList
  \param tCtr (int *) index of most recently added triangle to triList
  \param N_Div (int) number of edge divides
  \param n1,n2,n3 (int) indices in nodeList corresponding to three nodes of triangle being tessellated
  \return void (but nodeList and triList updated)

  Written by Brenna Argall
 
*/
void SUMA_tesselate( float *nodeList, int *triList, int *nCtr, int *tCtr, int N_Div, int n0, int n1, int n2) {

   int i=0, j=0;
   int *edge01=NULL, *edge12=NULL, *edge20=NULL, *currFloor=NULL;
   static char FuncName[]={"SUMA_tesselate"};
  
   SUMA_ENTRY;

   edge01 = SUMA_divEdge( nodeList, nCtr, n0, n1, N_Div);
   edge12 = SUMA_divEdge( nodeList, nCtr, n2, n1, N_Div);
   edge20 = SUMA_divEdge( nodeList, nCtr, n0, n2, N_Div);
   if (!edge01 || !edge12 || !edge20) {
      fprintf (SUMA_STDERR, "Error %s: Failed in SUMA_divEdge.\n", FuncName);
      SUMA_RETURNe;
   }
  
   currFloor = edge20;

   for (i=1; i<N_Div; ++i) {
      SUMA_triangulateRow( nodeList, triList, nCtr, tCtr, N_Div-i, currFloor, edge01[i], edge12[i]);
   }
  
   SUMA_addTri( triList, tCtr, currFloor[1], n1, currFloor[0]);

   if (edge01) SUMA_free(edge01);
   if (edge12) SUMA_free(edge12);
   if (edge20) SUMA_free(edge20);

   SUMA_RETURNe;
}

/*!
  edge = SUMA_divEdge( nodeList, nCtr, node1, node2, N_Div);
  
  Divides an edge defined by node1-node2 into N_Div segments.
  \param nodeList (float *) 3 x N_Node list of nodes
  \param nCtr (int *) current number of elements in nodeList
  \param node1, node2 (int) nodes defining edge being divided
  \param N_Div (int) number of segments edge divided into
  \return edge (int *) N_Div+1 list of nodes on edge (after segmentation)

  Written by Brenna Argall
*/
int * SUMA_divEdge( float *nodeList, int *nCtr, int node1, int node2, int N_Div) {

   float *newNodes = NULL;
   float n1[3], n2[3];
   int *edge = NULL;
   int i=0, j=0, k=0, m=0;
   int currIndex = (nCtr[0]-2)/3;
   static char FuncName[]={"SUMA_divEdge"};
  
   SUMA_ENTRY;
 
  
   edge = (int *) SUMA_calloc(N_Div+1, sizeof(int));
   newNodes = (float *)SUMA_calloc (3*(N_Div-1), sizeof(float));
  
   if (!edge || !newNodes) {
      fprintf (SUMA_STDERR, "Error %s: Failed to allocate.\n", FuncName);
      SUMA_RETURN (edge);
   }
  
   for(i=0; i<N_Div+1; ++i) {
      edge[i] = -1;
   }
  
   edge[0] = node1;  edge[N_Div] = node2;

   n1[0] = nodeList[3*node1];  n1[1] = nodeList[3*node1+1];  n1[2] = nodeList[3*node1+2];
   n2[0] = nodeList[3*node2];  n2[1] = nodeList[3*node2+1];  n2[2] = nodeList[3*node2+2];

   /*create new nodes*/
   for(i=0; i<N_Div-1; ++i) {
      j = 3*i;
      newNodes[j] =   ((i+1.0)/(float)N_Div)*(n2[0]-n1[0]) + n1[0];
      newNodes[j+1] = ((i+1.0)/(float)N_Div)*(n2[1]-n1[1]) + n1[1];
      newNodes[j+2] = ((i+1.0)/(float)N_Div)*(n2[2]-n1[2]) + n1[2];
   }

   /*check for existing nodes*/
   for (i=0; i<=currIndex; ++i) {
      j = 3*i;
      for (m=0; m<N_Div-1; ++m) {
         k = 3*m;
         if ( fabs(nodeList[j]-newNodes[k])<ep && fabs(nodeList[j+1]-newNodes[k+1])<ep && 
              fabs(nodeList[j+2]-newNodes[k+2])<ep ) {
            edge[m+1] = i;
         }
      }
   }

   for (i=1; i<N_Div; ++i) {
      if (edge[i]==-1) {
         SUMA_addNode( nodeList, nCtr, newNodes[3*(i-1)], newNodes[3*(i-1)+1], newNodes[3*(i-1)+2]);
         edge[i] = (nCtr[0]-2)/3;
      }
   }

   if (newNodes) SUMA_free(newNodes);
   
   SUMA_RETURN  (edge);
}

/*!
  SUMA_triangulateRow (nodeList, triList, nCtr, tCtr, N_Div, currFloor, node1, node2);

  Creates triangulation between line segments currFloor and node1-node2.  It is expected that node1-node2 has one fewer node than currFloor.
  \param nodeList (float *) 3 x N_Node list of nodes
  \param triList (int *) 3 x N_Tri list of node indices corresponding to triangles
  \param nCtr (int *) current number of elements in nodeList
  \param tCtr (int *) current number of elements in triList
  \param N_Div (int) number of divisions to be created from line segment node1-node2
  \param currFloor (int *) vector containing nodes of line segment "below" segment node1-node2 (length N_Div+1)
  \param node1, node2 (int) nodeList indices of nodes defining segment "above" currFloor
  \return void (but triList and nodeList updated)

  Written by Brenna Argall
*/
/*see LNB p28 for diagram*/
void SUMA_triangulateRow( float *nodeList, int *triList, int *nCtr, int *tCtr, int N_Div, int *currFloor, int node1, int node2) {
  
   int i=0, j=0;
   float n1[3], n2[3], newNode[3];
   int  *newArray = NULL;
   static char FuncName[]={"SUMA_triangulateRow"};
  
   SUMA_ENTRY;

   newArray = (int *)SUMA_calloc(N_Div+1, sizeof(int));
   if (!newArray) {
      fprintf (SUMA_STDERR, "Error %s: Failed to allocate.\n", FuncName);
      SUMA_RETURNe;
   }
   
   n1[0] = nodeList[3*node1];  n1[1] = nodeList[3*node1+1];  n1[2] = nodeList[3*node1+2];
   n2[0] = nodeList[3*node2];  n2[1] = nodeList[3*node2+1];  n2[2] = nodeList[3*node2+2];
   newArray[0] = node1;  newArray[N_Div] = node2;

   SUMA_addTri( triList, tCtr, currFloor[1], currFloor[0], newArray[0]);

   for (i=1; i<N_Div; ++i) {
      newNode[0] = ((float)i/(float)N_Div)*(n2[0]-n1[0]) + n1[0];
      newNode[1] = ((float)i/(float)N_Div)*(n2[1]-n1[1]) + n1[1];
      newNode[2] = ((float)i/(float)N_Div)*(n2[2]-n1[2]) + n1[2];
  
      SUMA_addNode( nodeList, nCtr, newNode[0], newNode[1], newNode[2]);
      newArray[i] = (nCtr[0]-2)/3;
      SUMA_addTri( triList, tCtr, newArray[i-1], currFloor[i], newArray[i]);
      SUMA_addTri( triList, tCtr, currFloor[i+1], newArray[i], currFloor[i]);
   }
   SUMA_addTri( triList, tCtr, newArray[N_Div-1], currFloor[N_Div], newArray[N_Div]);
   SUMA_addTri( triList, tCtr, newArray[N_Div], currFloor[N_Div+1], currFloor[N_Div]);

   for (i=0; i<N_Div+1; ++i) {
      currFloor[i] = newArray[i];
   }

   if (newArray) SUMA_free(newArray);

   SUMA_RETURNe;
}


/*!
  SUMA_addNode(nodeList, ctr, x, y, z);

  Function to add the x, y, z coordinates of a node to nodeList.
  \param nodeList (float *) 3 x N_node array of x,y,z coordinates of nodes
  \param ctr (int *) current position in nodeList
  \param x, y, z (float) x, y, z values of added node

*/
void SUMA_addNode(float *nodeList, int *ctr, float x, float y, float z) {
  
   static char FuncName[]={"SUMA_addNode"};
  
   SUMA_ENTRY;
  
   ++*ctr;
   nodeList[*ctr] = x;  
   ++*ctr;
   nodeList[*ctr] = y;  
   ++*ctr;
   nodeList[*ctr] = z;

   SUMA_RETURNe;
}

/*!
  SUMA_addTri(triList, ctr, n1, n2, n3);

  Function to add the three nodes of a triangle to triList.
  \param triList (int *) 3 x N_tri array of node indices creating triangles
  \param ctr (int *) current position in triList
  \param n1, n2, n3 (int *) nodeList indices of nodes creating added triangle
*/
void SUMA_addTri(int *triList, int *ctr, int n1, int n2, int n3) {

   static char FuncName[]={"SUMA_addTri"};
  
   SUMA_ENTRY;

   ++*ctr;
   triList[*ctr] = n1;
   ++*ctr;
   triList[*ctr] = n2;
   ++*ctr;
   triList[*ctr] = n3;

   SUMA_RETURNe;
}

/* See also SUMA_ProjectSurfaceToSphere */
SUMA_Boolean SUMA_ProjectToSphere(SUMA_SurfaceObject *SO, float *ctr, float r)
{
   static char FuncName[]={"SUMA_ProjectToSphere"};
   int i, i3;
   float dv, uv[3], U[2][3]={ {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0} }, *p1;
   SUMA_Boolean LocalHead = NOPE;

   SUMA_ENTRY;

   for (i=0; i<SO->N_Node; ++i) {
      i3 = 3*i;
      p1 = &(SO->NodeList[i3]);
      /* SUMA_UNIT_VEC(ctr, p1, uv, dv); */
      uv[0] = p1[0] - ctr[0]; uv[1] = p1[1] - ctr[1]; uv[2] = p1[2] - ctr[2];
      SUMA_POINT_AT_DISTANCE(uv, ctr, r, U);
      SO->NodeList[i3  ] = U[0][0]; SO->NodeList[i3+1] = U[0][1]; SO->NodeList[i3+2] = U[0][2]; 
   }

   SO->isSphere = SUMA_GEOM_SPHERE;
   SO->SphereRadius = r;
   SUMA_COPY_VEC(ctr, SO->SphereCenter, 3, float, float);
   
   SUMA_RETURN(YUP);
}

/*!
  SO = SUMA_CreateIcosahedron (r, depth, ctr, bin, ToSphere);

  This function creates an icosahedron of size r and to tessellation extent depth.
  \param r (float) size of icosahedron (distance from center to node).
  \param depth (int) number of edge subdivisions (bin='n') or depth of recursive tessellation (bin='y')
  \param ctr (float[]) coordinates of center of icosahedron
  \param bin (char[]) indicates whether tessellation binary/recursive ('y') or brute ('n')
  \param ToSpHere (int) if 1 then project nodes to form a sphere of radius r
  \ret SO (SUMA_SurfaceObject *) icosahedron is a surface object structure.
  returns NULL if function fails.
  SO returned with NodeList, N_Node, List, N_FaceSet, and NodeNormList
     
  Written by Brenna Argall  
*/
SUMA_SurfaceObject * SUMA_CreateIcosahedron (float r, int depth, float *ctru, char bin[], int ToSphere) 
{
   static char FuncName[]={"SUMA_CreateIcosahedron"};
   SUMA_SurfaceObject *SO = NULL;
   int i, numNodes=0, numTri=0, j, i3;
   float a,b, lgth, ctr[3]={0.0, 0.0, 0.0};
   int nodePtCt, triPtCt, *icosaTri=NULL;
   float *icosaNode=NULL;
   SUMA_SURF_NORM SN;
   SUMA_NODE_FIRST_NEIGHB *firstNeighb=NULL;
   SUMA_Boolean DoWind = YUP;
   int n=0, m=0, in=0, trouble;
   SUMA_Boolean LocalHead = NOPE;
   
   SUMA_ENTRY;
   
   if (ctru) { ctr[0] = ctru[0]; ctr[1] = ctru[1]; ctr[2] = ctru[2]; }
    
   SO = SUMA_Alloc_SurfObject_Struct(1);
   if (SO == NULL) {
      fprintf (SUMA_STDERR,"Error %s: Failed to allocate for Surface Object.", FuncName);
      SUMA_RETURN (NULL);
   }  

   
   if (strcmp(bin, "y") == 0) { numTri = 20*pow(2,2*depth); }  //exact
   else {
      if (depth !=0) {  numTri = 20*pow(depth, 2); }
      else numTri = 20;
   }
   if (depth != 0) {  numNodes = 3*numTri; }  //conservative
   else numNodes = 12;
     

   if (LocalHead) fprintf(SUMA_STDERR,"%s: Allocated for %d Nodes, %d numTri\n", FuncName, numNodes, numTri);
   
   /**icosahedron creation and tessellation*/
   SUMA_ICOSAHEDRON_DIMENSIONS(r, a, b, lgth); /* lgth is the length of edge by dist node0->node1 */
   
   if (LocalHead) {
      fprintf(SUMA_STDERR,"%s: a = %f, b=%f, rad = %f, lgth = %f\nctr = [%f %f %f]\n", FuncName, a, b, r, lgth, ctr[0], ctr[1], ctr[2]);
   }
   /*assign ep to be 1/2 the lenth of the maximum final distance between two nodes
     (see LNB p3 / p29)*/
   if (strcmp(bin, "y") == 0) {
      ep = lgth / pow(2, depth+1);
   }
   else ep = lgth / (2*depth);

   /**create icosahedron node list*/
   nodePtCt = -1;
   icosaNode = (float *) SUMA_calloc(3*numNodes, sizeof(float));
   icosaTri = (int *) SUMA_calloc(3*numTri, sizeof(int));

   if (!icosaNode || !icosaTri) {
      fprintf (SUMA_STDERR,"Error %s: Could not allocate for icosaNode and/or icosaTri.\n",FuncName);
      SUMA_Free_Surface_Object (SO);
      SUMA_RETURN (NULL); 
   }

   SUMA_addNode( icosaNode, &nodePtCt, 0+ctr[0], b+ctr[1], -a+ctr[2] );  
   SUMA_addNode( icosaNode, &nodePtCt, 0+ctr[0], b+ctr[1], a+ctr[2] );
   SUMA_addNode( icosaNode, &nodePtCt, 0+ctr[0], -b+ctr[1], a+ctr[2] );  
   SUMA_addNode( icosaNode, &nodePtCt, 0+ctr[0], -b+ctr[1], -a+ctr[2] );
   SUMA_addNode( icosaNode, &nodePtCt, -b+ctr[0], a+ctr[1], 0+ctr[2] );  
   SUMA_addNode( icosaNode, &nodePtCt, -b+ctr[0], -a+ctr[1], 0+ctr[2] );
   SUMA_addNode( icosaNode, &nodePtCt, b+ctr[0], a+ctr[1], 0+ctr[2] );   
   SUMA_addNode( icosaNode, &nodePtCt, b+ctr[0], -a+ctr[1], 0+ctr[2] );
   SUMA_addNode( icosaNode, &nodePtCt, a+ctr[0], 0+ctr[1], b+ctr[2] );   
   SUMA_addNode( icosaNode, &nodePtCt, -a+ctr[0], 0+ctr[1], -b+ctr[2] );
   SUMA_addNode( icosaNode, &nodePtCt, -a+ctr[0], 0+ctr[1], b+ctr[2] );  
   SUMA_addNode( icosaNode, &nodePtCt, a+ctr[0], 0+ctr[1], -b+ctr[2] );

   /**tessellate icosahedron*/

   triPtCt = -1;

   /**if recursion depth is 0, just make icosahedron (no tessellation)*/
   if (depth==0) {

      SUMA_addTri( icosaTri, &triPtCt, 0, 4, 6 );   
      SUMA_addTri( icosaTri, &triPtCt, 1, 6, 4 );
      
      SUMA_addTri( icosaTri, &triPtCt, 0, 9, 4 );   
      SUMA_addTri( icosaTri, &triPtCt, 1, 8, 6 );
      
      SUMA_addTri( icosaTri, &triPtCt, 0, 3, 9 );   
      SUMA_addTri( icosaTri, &triPtCt, 1, 2, 8 );
      
      SUMA_addTri( icosaTri, &triPtCt, 0, 11, 3 );  
      SUMA_addTri( icosaTri, &triPtCt, 1, 10, 2 );
      
      SUMA_addTri( icosaTri, &triPtCt, 0, 6, 11 );  
      SUMA_addTri( icosaTri, &triPtCt, 1, 4, 10 );
      
      SUMA_addTri( icosaTri, &triPtCt, 2, 7, 8 );   
      SUMA_addTri( icosaTri, &triPtCt, 3, 11, 7 );
      
      SUMA_addTri( icosaTri, &triPtCt, 2, 5, 7 );   
      SUMA_addTri( icosaTri, &triPtCt, 3, 7, 5 );
      
      SUMA_addTri( icosaTri, &triPtCt, 2, 10, 5 );  
      SUMA_addTri( icosaTri, &triPtCt, 3, 5, 9 );
      
      SUMA_addTri( icosaTri, &triPtCt, 4, 9, 10 );  
      SUMA_addTri( icosaTri, &triPtCt, 6, 8, 11 );
      
      SUMA_addTri( icosaTri, &triPtCt, 5, 10, 9 );  
      SUMA_addTri( icosaTri, &triPtCt, 7, 11, 8 );
   }

   else {
      if (strcmp(bin, "y") == 0) {
         /*binary tessellation*/
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 0, 4, 6);
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 0, 9, 4 );
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 0, 3, 9 );
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 0, 11, 3 );
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 0, 6, 11 );
       
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 1, 6, 4 );
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 1, 8, 6 );
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 1, 2, 8 );
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 1, 10, 2 );
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 1, 4, 10 );
       
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 2, 7, 8 );
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 2, 5, 7 );
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 2, 10, 5 );
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 4, 9, 10 );
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 5, 10, 9 );
       
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 3, 11, 7 );
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 3, 7, 5 );
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 3, 5, 9 );
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 6, 8, 11 );
         SUMA_binTesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 7, 11, 8 );
      }

      else {
         /*brute tessellation*/
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 0, 4, 6);
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 0, 9, 4 );
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 0, 3, 9 );
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 0, 11, 3 );
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 0, 6, 11 );
       
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 6, 4 );
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 8, 6 );
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 2, 8 );
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 10, 2 );
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 1, 4, 10 );
       
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 2, 7, 8 );
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 2, 5, 7 );
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 2, 10, 5 );
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 4, 9, 10 );
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 5, 10, 9 );
       
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 3, 11, 7 );
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 3, 7, 5 );
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 3, 5, 9 );
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 6, 8, 11 );
         SUMA_tesselate(icosaNode, icosaTri, &nodePtCt, &triPtCt, 
                        depth, 7, 11, 8 );
      }
   }

   numNodes = (nodePtCt+1)/3;
   numTri = (triPtCt+1)/3;

   if (LocalHead) 
      fprintf( SUMA_STDERR,
               "%s: There are %d nodes, %d triangles in the icosahedron.\n", 
               FuncName, numNodes, numTri);

   /* store in SO and get out */
   SO->isSphere = SUMA_GEOM_ICOSAHEDRON;
   SUMA_COPY_VEC(ctr,SO->SphereCenter,3, float, float);
   SUMA_COPY_VEC(ctr,SO->Center,3, float, float);/* ZSS: This had not been set.
                                                    Affects 3dSkullStrip after 
                                                    addition of SO->SphereCenter
                                                    Sept. 08 */
   SO->SphereRadius = r;
   SO->NodeList = icosaNode;
   SO->FaceSetList = icosaTri;
   SO->N_Node = numNodes;
   SO->N_FaceSet = numTri;
   SO->NodeDim = 3;
   SO->FaceSetDim = 3;
   SO->normdir = 1;
   SO->idcode_str = (char *)SUMA_calloc (SUMA_IDCODE_LENGTH, sizeof(char));   
   UNIQ_idcode_fill (SO->idcode_str);
   
   /* check the winding ? */
   if (DoWind) {
      if (LocalHead) 
         fprintf(SUMA_STDOUT, "%s: Making Edge list ....\n", FuncName); 
      SO->EL = SUMA_Make_Edge_List_eng (SO->FaceSetList, SO->N_FaceSet, 
                                        SO->N_Node, SO->NodeList, 0, 
                                        SO->idcode_str);
      if (SO->EL == NULL) {
         fprintf( SUMA_STDERR, 
                  "Error %s: Failed in SUMA_Make_Edge_List.\n"
                  "Neighbor list will not be created\n", FuncName);
         SUMA_Free_Surface_Object (SO);
         SUMA_RETURN (NULL);
      } else {
      }
      
      if (!SUMA_MakeConsistent ( SO->FaceSetList, SO->N_FaceSet, 
                                 SO->EL, 0, &trouble)) {
         fprintf( SUMA_STDERR,
                  "Error %s: Failed in SUMA_MakeConsistent.\n", FuncName);
         SUMA_Free_Surface_Object (SO);
         SUMA_RETURN (NULL);
      }
      else {
         if (LocalHead) 
            fprintf( SUMA_STDERR,
                     "%s: Eeeexcellent. All triangles consistent.\n", FuncName);
      }
      /* determine the MemberFaceSets */
      if (LocalHead) 
         fprintf(SUMA_STDOUT, "%s: Determining MemberFaceSets  ...\n", FuncName);
      SO->MF = SUMA_MemberFaceSets(SO->N_Node, SO->FaceSetList,
                                   SO->N_FaceSet, SO->FaceSetDim, 
                                   SO->idcode_str);
      if (SO->MF->NodeMemberOfFaceSet == NULL) {
         fprintf( SUMA_STDERR,
                  "Error %s: Error in SUMA_MemberFaceSets\n", FuncName);
         SUMA_Free_Surface_Object (SO); /* that takes care of freeing 
                                           leftovers in MF */
         SUMA_RETURN (NULL);
      }else { /* create Inode to avoid whining upon cleanup */
      }
      
      
   }
   
   /* project to sphere ? */
   if (ToSphere) {
      if (!SUMA_ProjectToSphere(SO, ctr, r)) {
         SUMA_S_Err("Failed to project to sphere.");
         SUMA_RETURN(NULL);
      }  
   }
   
   /* create surface normals */
   SN = SUMA_SurfNorm( SO->NodeList, SO->N_Node, SO->FaceSetList, SO->N_FaceSet);
   SO->NodeNormList = SN.NodeNormList;
   SO->FaceNormList = SN.FaceNormList;

   /*create first neighbor list*/
   if (!SO->EL) 
      SO->EL = SUMA_Make_Edge_List (SO->FaceSetList, SO->N_FaceSet, 
                                    SO->N_Node, SO->NodeList, SO->idcode_str);
   
   SO->FN = SUMA_Build_FirstNeighb( SO->EL, SO->N_Node, SO->idcode_str, 1);
   if(SO->FN==NULL) {
      fprintf(SUMA_STDERR, 
               "Error %s: Failed in creating neighb list.\n", FuncName);
   }
   
   SUMA_RETURN (SO);
}

/*!
  SUMA_Boolean = SUMA_inNodeNeighb( surf, nodeList, node, PO, P1);

  Determines whether or not point P1 is inside of triangles of which node[0] or [1] or [2] is a node.
  \param surf (SUMA_SurfaceObject) surface being intersected by P1
  \param nodeList (float *) 3 x N_Node vector of nodes in surface (pass as NULL if equals surf->NodeList)
  \param node (int *) vector to contain 3 nodes of intersected triangle,
  originally contains three nodes to work with. if you 
  want only 1 or 2 nodes examined, use node[1] = -1 or 
  node[2] = -1, respectively
  \param PO (float *) point to form ray with P1 st ray slope = node normal of P1
  \param P1 (float *) intersecting point in question; if not on surface, returned with point where ray intersects surface
  \ret found (SUMA_Boolean) true if P1 in triangle with node[0] as a node
  
  Written by Ziad Saad / Brenna Argall
*/

SUMA_Boolean SUMA_inNodeNeighb( SUMA_SurfaceObject *surf, float *nodeList, 
                                int *node, float *P0, float *P1) {

   int i=0, j=0, k=0, examinedNum=0;
   SUMA_Boolean found=NOPE;
   float hitOnSurf[3];
   int  incidentTri[100], N_incident = 0, itry;
   int examinedTri[100], ifound, i_node0 = -1, i_node1 = -1, i_node2 = -1;
   SUMA_Boolean LocalHead = NOPE;
   static char FuncName[]={"SUMA_inNodeNeighb"};
   
   SUMA_ENTRY;
   
   if (nodeList==NULL) {
      fprintf (SUMA_STDERR, 
         "Warning %s: Assigning surf->NodeList to nodeList.\n", FuncName); 
      nodeList = surf->NodeList;
   }

   if (LocalHead) fprintf(SUMA_STDERR, "%s: P0-P1 [%f, %f, %f] - [%f, %f, %f]\n",
                          FuncName, P0[0], P0[1], P0[2], P1[0], P1[1], P1[2]);

   found = NOPE;
   itry = 0;
   examinedNum = 0;
   while (itry < 3 && node[itry] >= 0 && !found) {
      if (LocalHead) 
         fprintf(SUMA_STDERR, "%s: Trying neighbors of node %d.\n", 
                 FuncName, node[itry]);
      i = 0;
      while ((i < surf->FN->N_Neighb[node[itry]] ) && !found) { 
         if (!SUMA_Get_Incident( node[itry], 
                                 surf->FN->FirstNeighb[node[itry]][i], surf->EL, 
                                 incidentTri, &N_incident, 1, 0)) {
            fprintf (SUMA_STDERR,
                     "Error %s: Failed in SUMA_Get_Incident.\n", FuncName);
            SUMA_RETURN (NOPE);
         }

         /**check triangles incident to current edge*/
         j = 0;
         while ((j < N_incident) && !found) {

            /**triangle in list?*/
            SUMA_IS_IN_VEC(examinedTri, examinedNum, incidentTri[j], ifound);
            
            /**if not found , add index to list and test for intersection*/
            if (ifound < 0) {
               examinedTri[examinedNum] = incidentTri[j];
               ++examinedNum;

               i_node0 = surf->FaceSetList[ 3*incidentTri[j] ];
               i_node1 = surf->FaceSetList[ 3*incidentTri[j]+1 ];
               i_node2 = surf->FaceSetList[ 3*incidentTri[j]+2 ];

               if (SUMA_MT_isIntersect_Triangle (P0, P1, 
                        &(nodeList[3*i_node0]), &(nodeList[3*i_node1]), 
                        &(nodeList[3*i_node2]), hitOnSurf, NULL, NULL)) {
                  found = YUP;
                  node[0] = i_node0;
                  node[1] = i_node1;
                  node[2] = i_node2;
                  if (LocalHead) {
                     fprintf(SUMA_STDERR, 
            "%s: Triangle %d [%d, %d, %d] is intersected at (%f, %f, %f)\n", 
                             FuncName, incidentTri[j], node[0], node[1], node[2],
                             hitOnSurf[0], hitOnSurf[1], hitOnSurf[2]);
                     fprintf(SUMA_STDERR, 
            "%s: Coordinates of nodes forming triangle are:\n", FuncName);
                     fprintf(SUMA_STDERR, "%f, %f, %f\n", 
                             nodeList[3*i_node0], 
                             nodeList[3*i_node0+1], nodeList[3*i_node0+2]);
                     fprintf(SUMA_STDERR, "%f, %f, %f\n", 
                             nodeList[3*i_node1], 
                             nodeList[3*i_node1+1], nodeList[3*i_node1+2]);
                     fprintf(SUMA_STDERR, "%f, %f, %f\n", 
                             nodeList[3*i_node2], 
                             nodeList[3*i_node2+1], nodeList[3*i_node2+2]);
                  }  
#if 0 /* turn on to compare intersection results to those obtained with SUMA_MT_intersect_triangle */
                  {
                     /* try other (slower) method 
                        for intersection and compare results*/
                     SUMA_MT_INTERSECT_TRIANGLE *MTI;
                     MTI = SUMA_MT_intersect_triangle (P1, P0, nodeList, 
                                                surf->N_Node, surf->FaceSetList, 
                                                surf->N_FaceSet, NULL, 0);
                     if (MTI) {
                        if (LocalHead)
                           fprintf(SUMA_STDERR, 
         "%s: Meth2-Triangle %d [%d, %d, %d] is intersected at (%f, %f, %f)\n", 
                                   FuncName, MTI->ifacemin, 
                                   surf->FaceSetList[3*MTI->ifacemin], 
                                   surf->FaceSetList[3*MTI->ifacemin+1],
                                   surf->FaceSetList[3*MTI->ifacemin+2], 
                                   MTI->P[0], MTI->P[1], MTI->P[2]);  

                        if (MTI->N_hits) {
                           /* compare results */
                           if (MTI->ifacemin != incidentTri[j]) {
                              fprintf (SUMA_STDERR,
           "Error %s: Warning, mismatch in results of triangle intersection. "
           "This should not be\n", 
                                       FuncName);
                              exit(1);
                           }
                        }

                        MTI = SUMA_Free_MT_intersect_triangle(MTI);
                     } 

                  }
#endif  

                  P1[0] = hitOnSurf[0];  
                  P1[1] = hitOnSurf[1];  
                  P1[2] = hitOnSurf[2];
               }else {
                  if (LocalHead)
                     fprintf(SUMA_STDERR, 
                        "%s: Triangle %d [%d, %d, %d] is not intersected.\n",
                          FuncName, incidentTri[j], i_node0, i_node1, i_node2);
               } 
            }
            ++j;
         }
         ++i;
      }
      ++itry;   
   }
  
   SUMA_RETURN (found);
}


/*!
  weight = SUMA_detWeight ( node0, node1, node2, hitPt );

  This function determines the weight of each of three nodes on a given point based upon distance. 
  \param node0 (double[3]) contains x,y,z coordinates for first node
  \param node1 (double[3]) contains x,y,z coordinates for second node
  \param node2 (double[3]) contains x,y,z coordinates for third node
  \param ptHit (double[3]) contains x,y,z coordinates for point feeling weight
  \return weight (double[3]) contains weights for each node0, node1, node2

  Written by Brenna Argall
*/
float * SUMA_detWeight (float node0[3], float node1[3], float node2[3], float ptHit[3]) {

   int i=0;
   float triNode0[3], triNode1[3], triNode2[3];
   float p00[3], p01[3], p02[3];
   float p10[3], p11[3], p12[3];
   float p20[3], p21[3], p22[3];
   float tri0[3], tri1[3], tri2[3], triOrig[3];
   float s0=0, s1=0, s2=0, sOrig=0, A0=0, A1=0, A2=0, Aorig=0;
   float wsum=0, *weight=NULL;
   static char FuncName[]={"SUMA_detWeight"};
  
   SUMA_ENTRY;
  
   /*weights determined by linear interpolation based on areas of triangles resulting
     from lines parallel to edges of hit triangle and intersecting ptHit (see p6-12 LNB)*/
  
   p00[0] = node0[0];  p00[1] = node0[1];  p00[2] = node0[2];
   p11[0] = node1[0];  p11[1] = node1[1];  p11[2] = node1[2];
   p22[0] = node2[0];  p22[1] = node2[1];  p22[2] = node2[2];

   /**end points of parallel lines*/
 
   /** (nodes of subtriangle / associated with original node) */
   /** (p00,p01,p02 / triNode0), (p10,p11,p12 / triNode1), (p20,p21,p22 / triNode2)*/
   for (i=0; i<3; ++i) {
      /*assign p01*/
      if (p00[i]==p22[i]) { p01[i] = intersection_map( p11[i], p22[i], p00[i], p11[i], ptHit[i] ); }
      else { p01[i] = intersection_map( p11[i], p22[i], p11[i], p00[i], ptHit[i] ); }
      /*assign p02*/
      if (p11[i]==p00[i]) { p02[i] = intersection_map( p11[i], p22[i], p22[i], p00[i], ptHit[i] ); }
      else { p02[i] = intersection_map( p11[i], p22[i], p00[i], p22[i], ptHit[i] ); }
      /*assign p10*/
      if (p22[i]==p11[i]) { p10[i] = intersection_map( p22[i], p00[i], p00[i], p11[i], ptHit[i] ); }
      else { p10[i] = intersection_map( p22[i], p00[i], p11[i], p00[i], ptHit[i] ); }
      /*assign p12*/
      if (p11[i]==p00[i]) { p12[i] = intersection_map( p22[i], p00[i], p11[i], p22[i], ptHit[i] ); }
      else { p12[i] = intersection_map( p22[i], p00[i], p22[i], p11[i], ptHit[i] ); }
      /*assign p20*/
      if (p22[i]==p11[i]) { p20[i] = intersection_map( p00[i], p11[i], p22[i], p00[i], ptHit[i] ); }
      else { p20[i] = intersection_map( p00[i], p11[i], p00[i], p22[i], ptHit[i] ); }
      /*assign p21*/
      if (p00[i]==p22[i]) { p21[i] = intersection_map( p00[i], p11[i], p11[i], p22[i], ptHit[i] ); }
      else { p21[i] = intersection_map( p00[i], p11[i], p22[i], p11[i], ptHit[i] ); }
   }

   /**length of subtriangle edges*/

   tri0[0] = sqrt( pow(p01[0]-p00[0],2) + pow(p01[1]-p00[1],2) + pow(p01[2]-p00[2],2) );
   tri0[1] = sqrt( pow(p02[0]-p01[0],2) + pow(p02[1]-p01[1],2) + pow(p02[2]-p01[2],2) );
   tri0[2] = sqrt( pow(p00[0]-p02[0],2) + pow(p00[1]-p02[1],2) + pow(p00[2]-p02[2],2) );
  
   tri1[0] = sqrt( pow(p11[0]-p10[0],2) + pow(p11[1]-p10[1],2) + pow(p11[2]-p10[2],2) );
   tri1[1] = sqrt( pow(p12[0]-p11[0],2) + pow(p12[1]-p11[1],2) + pow(p12[2]-p11[2],2) );
   tri1[2] = sqrt( pow(p10[0]-p12[0],2) + pow(p10[1]-p12[1],2) + pow(p10[2]-p12[2],2) );
  
   tri2[0] = sqrt( pow(p21[0]-p20[0],2) + pow(p21[1]-p20[1],2) + pow(p21[2]-p20[2],2) );
   tri2[1] = sqrt( pow(p22[0]-p21[0],2) + pow(p22[1]-p21[1],2) + pow(p22[2]-p21[2],2) );
   tri2[2] = sqrt( pow(p20[0]-p22[0],2) + pow(p20[1]-p22[1],2) + pow(p20[2]-p22[2],2) );
  
   /**area of subtriangles*/
  
   s0 = .5*(tri0[0] + tri0[1] + tri0[2]);
   s1 = .5*(tri1[0] + tri1[1] + tri1[2]);
   s2 = .5*(tri2[0] + tri2[1] + tri2[2]);
  
   A0 = sqrt( s0*(s0-tri0[0])*(s0-tri0[1])*(s0-tri0[2]) );
   A1 = sqrt( s1*(s1-tri1[0])*(s1-tri1[1])*(s1-tri1[2]) );
   A2 = sqrt( s2*(s2-tri2[0])*(s2-tri2[1])*(s2-tri2[2]) );

   /*length of edges and area of original triangle*/

   triOrig[0] = sqrt( pow(p11[0]-p00[0],2) + pow(p11[1]-p00[1],2) + pow(p11[2]-p00[2],2) );
   triOrig[1] = sqrt( pow(p22[0]-p11[0],2) + pow(p22[1]-p11[1],2) + pow(p22[2]-p11[2],2) );
   triOrig[2] = sqrt( pow(p00[0]-p22[0],2) + pow(p00[1]-p22[1],2) + pow(p00[2]-p22[2],2) );

   sOrig = .5*(triOrig[0] + triOrig[1] + triOrig[2]);
   Aorig = sqrt( sOrig*(sOrig-triOrig[0])*(sOrig-triOrig[1])*(sOrig-triOrig[2]) );
  
   /**weights*/
   weight = (float *)SUMA_calloc( 3, sizeof(float) );
   weight[0] = (Aorig-A0)/Aorig;  weight[1] = (Aorig-A1)/Aorig;  weight[2] = (Aorig-A2)/Aorig;
   wsum = weight[0] + weight[1] + weight[2];
   weight[0] = weight[0]/wsum;  weight[1] = weight[1]/wsum;  weight[2] = weight[2]/wsum;
  
   //  fprintf(SUMA_STDERR, "weight: (%f, %f, %f)\n", weight[0], weight[1], weight[2]);
  
   SUMA_RETURN (weight);

} 



/**gives value for intersection of two lines, as defined in SUMA_MapSurface 
      (see p10 LNB)*/
float intersection_map(float a, float b, float c, float d, float val) {
  
   float sol = (val*(c-d) - d*(a-b)) / (c+b-a-d);

   return sol;
}



/*!
  MI = MapSurface (surf1, surf2);

  This function creates a mapping of one surface onto another (surfaces assumed to be spherical).
  \param surf1 (SUMA_SurfaceObject *) first surface of surface object structure
  \param surf2 (SUMA_SurfaceObject *) second surface of surface object structure
  \return MI (SUMA_MorphInfo *) contains information necessary to perform forwards and backwards morphing;
  returns NULL if function fails.
  MI returned with N_Node, N_FaceSet, Weight, ClsNodes and FaceSetList.

  Written by Brenna Argall
*/

SUMA_MorphInfo * SUMA_MapSurface (SUMA_SurfaceObject *surf1, 
                                  SUMA_SurfaceObject *surf2,
                                  int verb, char * dist_prefix )
{
   static char FuncName[]={"SUMA_MapSurface"};

   /**surf1 variables*/
   int numNodes_1=0, numFace_1=0;
   float *nodeList_1=NULL, *ctrNodeList_1=NULL;
   int *faceList_1=NULL;

   /**surf2 variables*/
   int numNodes_2=0, numFace_2=0;
   float *nodeList_2=NULL, *ctrNodeList_2=NULL;
   int *faceList_2=NULL;

   int i=0, j=0, k=0, m=0, j_srtd;
   float *weight=NULL;
   int *clsNodes=NULL;
   SUMA_MorphInfo *MI;
   double dr2, dmin, dmax, dval, dctr[3];
   float ctr1[3], ctr2[3], zero[3]={0.0, 0.0, 0.0}, r2, dist_tmp;
   float  *justX_2=NULL, *justX_1=NULL, *srtdX_ctrNodeList_2=NULL;
   int *i_SrtdX_2=NULL;
   int N_outliers;
   float currNode[3], ptHit[3], currDist=0, avgDist=0.0, pi=3.14159265359;
   int seg[2], i_node[3];
   float min_dist[3], curr_restr;

   SUMA_Boolean found=NOPE;
   float *triNode0, *triNode1, *triNode2, weight_tot;
   SUMA_SO_map *SO=NULL;
   SUMA_Boolean LocalHead = NOPE;
   
   FILE * dfp=NULL;     /* can write distortions to this file */
   float  ddiff, ratio;

   SUMA_ENTRY;


   MI = SUMA_Create_MorphInfo();
   if (MI == NULL) {
      fprintf (SUMA_STDERR,
               "Error %s: Failed to allocate for MorphInfo.\n", FuncName);
      SUMA_RETURN (NULL);
   }  

   /**assign surf1 variables*/
   nodeList_1 = surf1->NodeList;
   faceList_1 = surf1->FaceSetList;
   numNodes_1 = surf1->N_Node;
   numFace_1 = surf1->N_FaceSet;
 
   /**assign surf2 variables*/
   nodeList_2 = surf2->NodeList;
   faceList_2 = surf2->FaceSetList;
   numNodes_2 = surf2->N_Node;
   numFace_2 = surf2->N_FaceSet;

   clsNodes = (int *)SUMA_calloc( 3*numNodes_1, sizeof(int) );
   weight = (float *)SUMA_calloc( 3*numNodes_1, sizeof(float) );
   if (!clsNodes || !weight) {
      if (clsNodes) SUMA_free(clsNodes);
      if (weight) SUMA_free(weight);
      fprintf (SUMA_STDERR,
               "Error %s: Failed to allocate for clsNodes || weight.\n", 
               FuncName);
      SUMA_RETURN (NULL);
   }

   /* ----------------------------------------------------------------------
      change to centering:                      17 Mar 2017 [rickr]

      Originally the surfaces were recentered to match that of surf2.  But
      the step of scaling surf1 coordinates to land near surf2 did not deal
      with non-zero centers properly, e.g.  
         ptHit[0] = (r2/currDist)*currNode[0];
      should have read:
         ptHit[0] = (r2/currDist)*(currNode[0]-zero[0]) + zero[0];
      where ratio=r2/currDist is the ratio of distances to the centers.

      The difference: zero * (1 - ratio).

      For icosahedrons, ratio tends to vary between 0.7 and 1 (where it hits
      the surface), so each coordinate is distorted by up to .3 * the center
      coordinate (and they should be accurate where the ico hit the sphere).

      The change: instead of fixing ptHit, recenter both surfaces at 0,0,0.
      That simplifies (and speeds up) the computations, and we don't care
      about the actual coordinates anyway, since the output is a mapping of
      ico index to 3 orig surf indices (of a triangle) and their relative
      weights.

      ---

      The effect of this was seen when adding the -NIFTI (or similar) option
      to @SUMA_Make_Spec_FS.  In that case, ConvertSurface would be run with
      the option '-ixmat_1D xmatras.1D', shifting the coordinates (and centers)
      of all surfaces to match the center of the NIFTI SurfVol dataset.  It
      looks like perhaps the original intention was to shift only anatomically
      correct surfaces, but it ended up being applied to all surfaces (as
      identical commands were run in the 'if' and 'else' conditions).

      That might have been okay, except that it exposed the distortion problem
      in this function because of shifting the sphere.reg.asc surfaces.

      ---

      This subtle problem was found by I DeWitt, and required quite a bit of
      investigation to diagnose.  Many thanks to Iain.
      ----------------------------------------------------------------------*/
 

   /* extract or compute the centers of the surfaces */

   if (SUMA_IS_GEOM_SYMM(surf1->isSphere)) {
      SUMA_COPY_VEC(surf1->SphereCenter, ctr1, 3, float, float);
   } else {
      /* compute as double, then copy over   17 Mar 2017 [rickr] */
      dctr[0]=0; dctr[1]=0; dctr[2]=0;
      for (i=0; i<numNodes_1; ++i) {
         j = 3*i;
         dctr[0] = dctr[0] + nodeList_1[j];
         dctr[1] = dctr[1] + nodeList_1[j+1];
         dctr[2] = dctr[2] + nodeList_1[j+2];
      }
      /* but still store in the float array */
      ctr1[0] = dctr[0]/numNodes_1;
      ctr1[1] = dctr[1]/numNodes_1;
      ctr1[2] = dctr[2]/numNodes_1;
   }
   if (SUMA_IS_GEOM_SYMM(surf2->isSphere)) {
      SUMA_COPY_VEC(surf2->SphereCenter, ctr2, 3, float, float);
   } else {
      /* first find centers of each surface (as double 17 Mar 2017) */
      dctr[0]=0; dctr[1]=0; dctr[2]=0;
      for (i=0; i<numNodes_2; ++i) {
         j = 3*i;
         dctr[0] = dctr[0] + nodeList_2[j];
         dctr[1] = dctr[1] + nodeList_2[j+1];
         dctr[2] = dctr[2] + nodeList_2[j+2];
      }
      /* again, store double results in float array */
      ctr2[0] = dctr[0]/numNodes_2;
      ctr2[1] = dctr[1]/numNodes_2;
      ctr2[2] = dctr[2]/numNodes_2;
   }

   /* obsolete: center surf1 to surf2 (that will make it easier to debug
    *           in SUMA) */

   /* INSTEAD, set both centers to 0,0,0 (below),  17 Mar 2017 [rickr] */
   /*      and zero is now just a permanent 0,0,0 array.  This will    */
   /*      make projections more straightforward, and the centers do   */
   /*      not affect the output anyway.                               */
 
   /* zero[0] = ctr2[0]; zero[1] = ctr2[1]; zero[2] = ctr2[2];         */
   
   ctrNodeList_1 = (float *) SUMA_calloc( 3*numNodes_1, sizeof(float) );
   ctrNodeList_2 = (float *) SUMA_calloc( 3*numNodes_2, sizeof(float) );
   if (!ctrNodeList_1 || !ctrNodeList_2) {
      if (ctrNodeList_1) SUMA_free(ctrNodeList_1);
      if (ctrNodeList_2) SUMA_free(ctrNodeList_2);
      if (clsNodes) SUMA_free(clsNodes);
      if (weight) SUMA_free(weight);
      if (i_SrtdX_2) SUMA_free(i_SrtdX_2);
      if (justX_2) SUMA_free(justX_2);
      fprintf (SUMA_STDERR,
               "Error %s: Failed to allocate for ctrNodeList_1 || "
               "ctrNodeList_2.\n", FuncName);
      SUMA_RETURN (NULL);
   }

   /* obsolete: one of these two loops will be useless if we stick to having 
                zero be the center of the one  of the two surfaces.... */
   /* NEW: adjust both so that each center is at 0,0,0, since once equated,
      the actual coordinates do not affect the output  17 Mar 2017 [rickr] */
   for (i=0; i<numNodes_1; ++i) {
      j = 3*i;
      ctrNodeList_1[j]   = nodeList_1[j]   - ctr1[0]; /* set center to 0,0,0 */
      ctrNodeList_1[j+1] = nodeList_1[j+1] - ctr1[1];
      ctrNodeList_1[j+2] = nodeList_1[j+2] - ctr1[2];
   }
   for (i=0; i<numNodes_2; ++i) {
      j = 3*i;
      ctrNodeList_2[j]   = nodeList_2[j]   - ctr2[0];
      ctrNodeList_2[j+1] = nodeList_2[j+1] - ctr2[1];
      ctrNodeList_2[j+2] = nodeList_2[j+2] - ctr2[2];
   }

   /* debug: note radius stats of surf 1 */
   if( verb ) {
      dr2 = 0.0;
      dmin = 99999.0;
      dmax = 0.0;
      for (i=0; i<numNodes_1; ++i) {
         j = 3*i;
         dval = sqrt( pow( ctrNodeList_1[j],   2) +
                      pow( ctrNodeList_1[j+1], 2) + 
                      pow( ctrNodeList_1[j+2], 2) );
         dr2 += dval;
         if( dval < dmin ) dmin = dval;
         if( dval > dmax ) dmax = dval;
      }
      dr2 /= numNodes_1;
      if( verb )
         fprintf(SUMA_STDERR,
                 "-- MI: surf 1 (%s) radius ...\n"
                 "       from center %f, %f, %f :\n"
                 "       min %f, mean %f, max %f\n",
                 surf1->Label ? surf1->Label : "<noname>",
                 ctr1[0], ctr1[1], ctr1[2], dmin, dr2, dmax);
   }

   /*find radius of surf2*/
   /*(in theory should be able to just take distance first node -> center, but 
      freesurfer surfs are not perfectly spherical)*/

   { /* no if(verb), but keep indentation for readability */
     /* or get function to return min, mean, max */
      dr2 = 0.0;
      dmin = 99999.0;
      dmax = 0.0;
      for (i=0; i<numNodes_2; ++i) {
         j = 3*i;
         dval = sqrt( pow( ctrNodeList_2[j], 2) + /* remove zero 17 Mar 2017 */
                      pow( ctrNodeList_2[j+1], 2) + 
                      pow( ctrNodeList_2[j+2], 2) );
         dr2 += dval;
         if( dval < dmin ) dmin = dval;
         if( dval > dmax ) dmax = dval;
      }
      /* compute as double, keep as float   19 Mar 2017 */
      dr2 /= numNodes_2;
      if( verb )
         fprintf(SUMA_STDERR,
                 "-- MI: surf 2 (%s) radius ...\n"
                 "       from center %f, %f, %f :\n"
                 "       min %f, mean %f, max %f\n",
                 surf2->Label ? surf2->Label : "<noname>",
                 ctr2[0], ctr2[1], ctr2[2], dmin, dr2, dmax);
   }

   /* actually keep this result in r2 (compute as double, keep as float) */
   r2 = dr2;

   avgDist = (4*pi*pow(r2,2))/numNodes_2;  /*average distance between nodes on 
                                             surf2 surface */
  

   /**make certain surf2 is spherical*/
   N_outliers = 0;
   for (i=0; i<numNodes_2; ++i) {
      j = 3*i;
      dist_tmp = sqrt(  pow( ctrNodeList_2[j], 2) +  /* no zero */
                        pow( ctrNodeList_2[j+1], 2) +
                        pow( ctrNodeList_2[j+2], 2) );
      if ( abs(dist_tmp-r2)>r2/10) {
         /*node does not lie on sphere*/
         if ( N_outliers>(numNodes_2/1000)) {
            /*too many outliers -> exit program*/
            fprintf(SUMA_STDERR, 
               "\nError %s: Too many outliers. "
               "Surface considered to be non-spherical.\n\n", FuncName);
            SUMA_RETURN(NULL);
         }
         fprintf( SUMA_STDERR, 
                  "Warning %s: Outlier detected! \n"
                  "Resetting to lie on sphere...\n", FuncName);
         N_outliers = N_outliers+1;
         ctrNodeList_2[j] = (r2/dist_tmp)*ctrNodeList_2[j];
         ctrNodeList_2[j+1] = (r2/dist_tmp)*ctrNodeList_2[j+1];
         ctrNodeList_2[j+2] = (r2/dist_tmp)*ctrNodeList_2[j+2];
      }
   }
      
   
   /**sort x of NodeList_2*/

   /*create array justX_2 of only X location values*/
   justX_2 = (float *) SUMA_calloc( numNodes_2, sizeof(float) );
   if (!justX_2 ) {
      fprintf (SUMA_STDERR,
               "Error %s: Failed to allocate for justX_2.\n", FuncName);
      if (ctrNodeList_1) SUMA_free(ctrNodeList_1);
      if (ctrNodeList_2) SUMA_free(ctrNodeList_2);
      if (clsNodes) SUMA_free(clsNodes);
      if (weight) SUMA_free(weight);
      SUMA_RETURN (NULL);
   }
  
   for (i=0; i<numNodes_2; ++i) {
      j = 3*i;
      justX_2[i] = ctrNodeList_2[j];
   }

   /*sort justX_2 */
   i_SrtdX_2 = SUMA_z_qsort( justX_2, numNodes_2 ); /*i_SrtdX_2 is array of 
                                 indices of justX_2 corresponding to sorting*/
                                             /*justX_2 is returned sorted*/
   if (!i_SrtdX_2) {
      fprintf (SUMA_STDERR,"Error %s: Failed in SUMA_z_qsort.\n", FuncName);
      if (ctrNodeList_1) SUMA_free(ctrNodeList_1);
      if (ctrNodeList_2) SUMA_free(ctrNodeList_2);
      if (clsNodes) SUMA_free(clsNodes);
      if (weight) SUMA_free(weight);
      if (justX_2) SUMA_free(justX_2);

      SUMA_RETURN (NULL);
   }

   /*create sorted ctrNodeList_2 based upon i_SrtdX_2*/
   srtdX_ctrNodeList_2 = SUMA_calloc( 3*numNodes_2, sizeof(float));
   for (i=0; i<numNodes_2; ++i) {
      j = 3*i;
      j_srtd = 3*i_SrtdX_2[i];
      srtdX_ctrNodeList_2[j]   = ctrNodeList_2[j_srtd];
      srtdX_ctrNodeList_2[j+1] = ctrNodeList_2[j_srtd+1];
      srtdX_ctrNodeList_2[j+2] = ctrNodeList_2[j_srtd+2];
   }


   /* allow users to write distortions out              27 Mar 2017 [rickr] */
   if( dist_prefix ) {
      char *label;
      char *dfname=NULL;
      int   slen;

      if( surf2->Label ) label = surf2->Label;
      else               label = "surf2labl";
      slen = strlen(dist_prefix) + strlen(label) + 6;
      dfname = malloc(slen * sizeof(char));
      sprintf(dfname, "%s.%s.txt", dist_prefix, label);
      fprintf(SUMA_STDERR,"++ %s: will write distortions to %s\n",
              FuncName, dfname);
      dfp = fopen(dfname, "w");
      if(!dfp) fprintf(SUMA_STDERR,"** failed to open %s for write\n",dfname);
      free(dfname);
   }


   /** mapping surf1 to surf2 */

   if (verb) fprintf(SUMA_STDERR,"\nComputing intersections...\n\n");
   ptHit[0]=0; ptHit[1]=0; ptHit[2]=0;
   triNode0=0; triNode1=0; triNode2=0;
 
   for (i=0; i<numNodes_1; ++i) {

      j=3*i; 
      currNode[0]=ctrNodeList_1[j];
      currNode[1]=ctrNodeList_1[j+1];
      currNode[2]=ctrNodeList_1[j+2];
      currDist = sqrt( pow( currNode[0], 2) +  /* removed zero  19 Mar 2017 */
                       pow( currNode[1], 2) + 
                       pow( currNode[2], 2) );

      ratio = r2/currDist; /* ratio of new to old distance from 0,0,0 */

      /*compute inflation of node onto sphere by adjusting surf1 node so 
         that its distance from zero[0],[1],[2]
         exactly equals the radius of the spherical surf2 (r2)*/
      /* should work properly now (with centers at 0,0,0), else would need
         rat*(cur-z)+z, say, to avoid distortion       18 Mar 2017 [rickr] */
      ptHit[0] = ratio*currNode[0];
      ptHit[1] = ratio*currNode[1];
      ptHit[2] = ratio*currNode[2];

      /* if writing distortions, do so here            27 Mar 2017 [rickr] */
      if( dfp ) {
         /* old centers were from ctr2
            distortion = z(1-rat) = ctr2[ind]*(1-r2/currDist), for ind=0..2 */
         fprintf(dfp, "%8.3f  %8.3f  %8.3f\n",
            ctr2[0]*(1.0-ratio),
            ctr2[1]*(1.0-ratio),
            ctr2[2]*(1.0-ratio));
      }

      /**find 3 nodes in ctrNodeList_2 closest to ptHit*/
      
      /*initialize variables*/
      found = NOPE;
      for (k=0; k<3; ++k) { 
         min_dist[k] = 2*r2;
         i_node[k] = -1;
      }
      curr_restr = (float)12.0*avgDist;  /*12.0 chosen by trial/error for best 
                                           timing compromise between 
                                           using expanded search vs brute force 
                                           for trouble nodes*/

      /*find placement of ptHit[0] in justX_2*/
      seg[0] = 0; 
      seg[1] = numNodes_2-1;

      if ( ptHit[0] < justX_2[seg[0]] )   /*note ptHit will be within r2/10 of 
                                    either of these values, so assignment is ok*/
         seg[1] = seg[0];                 /*(since ctrNodeList2 was adjusted to 
                                    have each distance within )*/
      else if ( ptHit[0] > justX_2[seg[1]] )  /*(r2/10 of r2, which was used to 
                                                scale ctrNodeList1, from which)*/
         seg[0] = seg[1];                      /*(justX_2 comes)*/
      else {
         if ( !SUMA_binSearch( justX_2, ptHit[0], seg, 0 )) {
            fprintf( SUMA_STDERR, 
                     "Error %s: Failed in binary search !(%f < %f < %f).\n\n", 
                     FuncName, justX_2[seg[0]], ptHit[0], justX_2[seg[1]]);
            if (ctrNodeList_1) SUMA_free(ctrNodeList_1);
            if (ctrNodeList_2) SUMA_free(ctrNodeList_2);
            if (clsNodes) SUMA_free(clsNodes);
            if (weight) SUMA_free(weight);
            if (i_SrtdX_2) SUMA_free(i_SrtdX_2);
            if (justX_2) SUMA_free(justX_2);
            if (srtdX_ctrNodeList_2) SUMA_free(srtdX_ctrNodeList_2);
            SUMA_RETURN (NULL);
         }
      }

      /*expand search segment*/
      while ( (ptHit[0] - srtdX_ctrNodeList_2[3*seg[0]]) < curr_restr 
               && seg[0]>0) { 
         if ( seg[0]>10 ) seg[0] = seg[0]-10; 
         else --seg[0];
      }
      while ( (srtdX_ctrNodeList_2[3*seg[1]] - ptHit[0]) < curr_restr 
               && seg[1]<(numNodes_2-1) ) { 
         if ( seg[1]<(numNodes_2-11) ) seg[1] = seg[1]+10;
         else ++seg[1]; 
      }

      /*search for 3 minimum distances to ptHit*/
      while ( !found && seg[1]-seg[0]<numNodes_2 && curr_restr<3*r2 ) { 
         /*3 min distances have not yet been found*/

         SUMA_Search_Min_Dist( ptHit, srtdX_ctrNodeList_2, seg, 
                               curr_restr, min_dist, i_node );
         
         if ( i_node[0]==-1 || i_node[1]==-1 || i_node[2]==-1 ) {
            /*sufficient (3) min_dist were not found -> 
               repeat and expand search of segment with more relaxed measures*/
            curr_restr = (float) 1.5*curr_restr;
            found = NOPE;
            while ( ptHit[0] - srtdX_ctrNodeList_2[3*seg[0]] < curr_restr 
                    && seg[0]>0) { 
               if (seg[0]>10) seg[0] = seg[0]-10; 
               else --seg[0];
            }
            while (  srtdX_ctrNodeList_2[3*seg[1]] - ptHit[0] < curr_restr && 
                     seg[1]<numNodes_2-1) { 
               if (k<numNodes_2-11) seg[1] = seg[1]+10;
               else ++seg[1]; 
            }
         }
         else found = YUP;
      }


      if ( i_node[0]==-1 || i_node[1]==-1 || i_node[2]==-1 ) {
         /*unable to acquire 3 closest nodes (???) -> exit*/
         fprintf( SUMA_STDERR, 
                  "Error %s: Unable to acquire 3 closest nodes ?!?\n\n", 
                  FuncName);
         if (ctrNodeList_1) SUMA_free(ctrNodeList_1);
         if (ctrNodeList_2) SUMA_free(ctrNodeList_2);
         if (clsNodes) SUMA_free(clsNodes);
         if (weight) SUMA_free(weight);
         if (i_SrtdX_2) SUMA_free(i_SrtdX_2);
         if (justX_2) SUMA_free(justX_2);
         if (srtdX_ctrNodeList_2) SUMA_free(srtdX_ctrNodeList_2);
         SUMA_RETURN (NULL);
      }

      /*translate back into unsorted ordering of ctrNodeList_2*/
      i_node[0] = i_SrtdX_2[i_node[0]];
      i_node[1] = i_SrtdX_2[i_node[1]];
      i_node[2] = i_SrtdX_2[i_node[2]];

      if (LocalHead) {
         fprintf(SUMA_STDERR,"----------------------------------------\n");
         fprintf(SUMA_STDERR, "%s: PtHit: [%f, %f, %f].\n", 
                               FuncName, ptHit[0], ptHit[1], ptHit[2]);
         fprintf(SUMA_STDERR, "%s: Node %d [%f, %f, %f], distances %f.\n", 
                 FuncName, i_node[0], ctrNodeList_2[3*i_node[0]], 
                 ctrNodeList_2[3*i_node[0]+1], ctrNodeList_2[3*i_node[0]+2], 
                 min_dist[0]);
         fprintf(SUMA_STDERR, "%s: Node %d [%f, %f, %f], distances %f.\n", 
                 FuncName, i_node[1], ctrNodeList_2[3*i_node[1]], 
                 ctrNodeList_2[3*i_node[1]+1], ctrNodeList_2[3*i_node[1]+2], 
                 min_dist[1]);
         fprintf(SUMA_STDERR, "%s: Node %d [%f, %f, %f], distances %f.\n", 
                 FuncName, i_node[2], ctrNodeList_2[3*i_node[2]], 
                 ctrNodeList_2[3*i_node[2]+1], ctrNodeList_2[3*i_node[2]+2], 
                 min_dist[2]);
         fprintf(SUMA_STDERR, "%s: orig ptHit (%f, %f, %f)\n", 
                  FuncName, ptHit[0], ptHit[1], ptHit[2]);
         fprintf(SUMA_STDERR, "%s: Trying 1- node %d\n", FuncName, i_node[0]);
      }  
      

      /**find nodes of intersected triangle*/

      if (surf2->FN == NULL) {
         fprintf(SUMA_STDERR, "%s: Surf2->FN is NULL.\n", FuncName);
         if (ctrNodeList_1) SUMA_free(ctrNodeList_1);
         if (ctrNodeList_2) SUMA_free(ctrNodeList_2);
         if (clsNodes) SUMA_free(clsNodes);
         if (weight) SUMA_free(weight);
         if (i_SrtdX_2) SUMA_free(i_SrtdX_2);
         if (justX_2) SUMA_free(justX_2);
         if (srtdX_ctrNodeList_2) SUMA_free(srtdX_ctrNodeList_2);
         SUMA_RETURN (NULL);
      }

      /* search neighborhoods of closest 3 nodes */
      found = SUMA_inNodeNeighb( surf2, ctrNodeList_2, i_node, zero, ptHit);

      if (!found) {
         /* try brute force */
         if (LocalHead) 
            fprintf(SUMA_STDERR, "%s: Trying Brute force. (%d)\n", FuncName, i);
         {
            SUMA_MT_INTERSECT_TRIANGLE *MTI;
         
            MTI = SUMA_MT_intersect_triangle(ptHit, zero, ctrNodeList_2, 
                                 numNodes_2, faceList_2, numFace_2, NULL, 0);
            if (MTI) {
               if (MTI->N_hits) {
                  if (LocalHead) 
                     fprintf( SUMA_STDERR, 
                              "%s: Brute force-Triangle %d [%d, %d, %d] \n"
                              "is intersected at (%f, %f, %f)\n", 
                         FuncName, MTI->ifacemin, 
                         surf2->FaceSetList[3*MTI->ifacemin], 
                         surf2->FaceSetList[3*MTI->ifacemin+1],
                         surf2->FaceSetList[3*MTI->ifacemin+2], 
                         MTI->P[0], MTI->P[1], MTI->P[2]);  
                  found = YUP;
                  ptHit[0] = MTI->P[0];
                  ptHit[1] = MTI->P[1];
                  ptHit[2] = MTI->P[2];
                  i_node[0] = surf2->FaceSetList[3*MTI->ifacemin];
                  i_node[1] = surf2->FaceSetList[3*MTI->ifacemin+1];
                  i_node[2] = surf2->FaceSetList[3*MTI->ifacemin+2];
               }
               MTI = SUMA_Free_MT_intersect_triangle(MTI);
            } 
         }
      }
   
      if (!found) {
         fprintf(SUMA_STDERR, 
                  "Error %s: !!!!!!!!!! intersected triangle not found.\n", 
                  FuncName);
         if (ctrNodeList_1) SUMA_free(ctrNodeList_1);
         if (ctrNodeList_2) SUMA_free(ctrNodeList_2);
         if (clsNodes) SUMA_free(clsNodes);
         if (weight) SUMA_free(weight);
         if (i_SrtdX_2) SUMA_free(i_SrtdX_2);
         if (justX_2) SUMA_free(justX_2);
         if (srtdX_ctrNodeList_2) SUMA_free(srtdX_ctrNodeList_2);
         SUMA_RETURN (NULL);
      } 
    
      if (LocalHead) 
         fprintf (SUMA_STDERR, 
                  "%s: (%d : %d : %d)\n  ptHit(%f, %f, %f)\n", 
                  FuncName, i_node[0], i_node[1], i_node[2], 
                  ptHit[0], ptHit[1], ptHit[2]);

      /**node indices of triangle intersected by ptHit*/
      clsNodes[j] = i_node[0];  
      clsNodes[j+1] = i_node[1];  
      clsNodes[j+2] = i_node[2];

      /** pointers to x,y,z of each node of intersected triangle*/
      triNode0 = &(ctrNodeList_2[ 3*i_node[0] ]);
      triNode1 = &(ctrNodeList_2[ 3*i_node[1] ]);
      triNode2 = &(ctrNodeList_2[ 3*i_node[2] ]);
    
      /**determine weights which are the barycetric coordinates 
         of the intersection node*/
      SUMA_TRI_AREA( ptHit, triNode1, triNode2, weight[j]); 
      SUMA_TRI_AREA( ptHit, triNode0, triNode2, weight[j+1]); 
      SUMA_TRI_AREA( ptHit, triNode0, triNode1, weight[j+2]); 
         /* if the index of the intersected triangle is very cheap to obtain, 
            you could set 
            weight[j+2] = SO->PolyArea[Face] - weight[j+1] - weight[j+0] 
            Of course, you must first compute PolyArea with 
            SUMA_SurfaceMetrics*/

      weight_tot = weight[j] + weight[j+1] + weight[j+2];
      if (weight_tot) {
         weight[j] /= weight_tot;
         weight[j+1] /= weight_tot;
         weight[j+2] /= weight_tot;
      }else { /* some triangles have zero area in FreeSurfer surfaces */
         weight[j] = weight[j+1] = weight[j+2] = 1.0/3.0;
      }

   } /* for each node */

   /* if distortions were written, close the file */
   if( dfp ) { fclose(dfp); dfp = NULL; }

   MI->N_Node_std = numNodes_1;
   MI->N_Node_orig = numNodes_2;
   MI->N_FaceSet_std = numFace_1;
   MI->Weight = weight;
   MI->ClsNodes = clsNodes;
   MI->FaceSetList = (int *) SUMA_calloc( 3*numFace_1, sizeof(int));
   if (!MI->FaceSetList) {
      fprintf( SUMA_STDERR, 
               "Error %s: Failed to allocate for MI->FaceSetList.\n", FuncName);
      if (ctrNodeList_1) SUMA_free(ctrNodeList_1);
      if (ctrNodeList_2) SUMA_free(ctrNodeList_2);
      if (clsNodes) SUMA_free(clsNodes);
      if (weight) SUMA_free(weight);
      if (i_SrtdX_2) SUMA_free(i_SrtdX_2);
      if (justX_2) SUMA_free(justX_2);
      if (srtdX_ctrNodeList_2) SUMA_free(srtdX_ctrNodeList_2);
      SUMA_RETURN (NULL);
   }
   for (i=0; i<numFace_1; ++i) {
      j = 3*i;
      MI->FaceSetList[j] = faceList_1[j];
      MI->FaceSetList[j+1] = faceList_1[j+1];
      MI->FaceSetList[j+2] = faceList_1[j+2];
   }

   if (ctrNodeList_1) SUMA_free(ctrNodeList_1);
   if (ctrNodeList_2) SUMA_free(ctrNodeList_2);
   if (i_SrtdX_2) SUMA_free(i_SrtdX_2);
   if (justX_2) SUMA_free(justX_2);
   if (srtdX_ctrNodeList_2) SUMA_free(srtdX_ctrNodeList_2);

   SUMA_RETURN (MI);
} 

 
/*!
  SUMA_Search_Min_dist( seg, pt, nodeList, restr, dist, i_dist)

  Function to search for three minimum distances between a given point and nodes within a given segment.
  \param pt (float *) Point to which distances are calculated (length is 3: x y z).
  \param nodeList (float *) Array (1D) of x,y,z values of nodes.
  \param seg (int *) Contains beginning and ending indices of search segment of nodeList.
  \param restr (float) Restriction distance for searching within each (x,y,z) dimension.
  \param dist (float *) Returned containing 3 minimum distances; may be passed already containing distances to be updated or as empty (but allocated for). If empty, default initializes to 3*pow(restr,2).
  \param i_dist (int *) Indices of nodes within nodeList from which distances contained in dist were calculated.
  \ret void
*/

void SUMA_Search_Min_Dist(  float* pt, float* nodeList, int* seg, 
                            float restr, float *dist, int *i_dist ) {

   static char FuncName[]={"SUMA_Search_Min_Dist"};
   float tempD;
   int j, k;

   SUMA_ENTRY;
   
   if ( !dist[0] || !dist[1] || !dist[2] ) {
      tempD = 3*pow(restr,2); 
      dist[0] = tempD;  dist[1] = tempD;  dist[2] = tempD;
      i_dist[0] = -1;   i_dist[1] = -1;   i_dist[2] = -1;
   }
   else tempD = dist[2]+1;

   for (k=seg[0]; k<=seg[1]; ++k) {
      j = 3*k;
      if (pt[0]-nodeList[j] < restr) {
         if (pt[0]-nodeList[j] > -restr) {
            if (pt[1]-nodeList[j+1] < restr) {
               if (pt[1]-nodeList[j+1] > -restr) {
                  if (pt[2]-nodeList[j+2] < restr) {
                     if (pt[2]-nodeList[j+2] > -restr) {
                        
                        tempD = sqrt(  pow(pt[0]-nodeList[j],2) + 
                                       pow(pt[1]-nodeList[j+1],2) + 
                                       pow(pt[2]-nodeList[j+2],2) );
                        
                        if (tempD < dist[2]) {
                           if (tempD < dist[1]) {
                              if (tempD < dist[0]) {
                                 dist[2] = dist[1];    i_dist[2] = i_dist[1];  
                                 dist[1] = dist[0];    i_dist[1] = i_dist[0]; 
                                 dist[0] = tempD;      i_dist[0] = k; 
                              }       
                              else {
                                 dist[2] = dist[1];    i_dist[2] = i_dist[1];
                                 dist[1] = tempD;      i_dist[1] = k;
                              }
                           } 
                           else {
                              dist[2] = tempD;  i_dist[2] = k;
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }

   SUMA_RETURNe;
}



/*!
  function used to create a SUMA_SO_map structure
*/
SUMA_SO_map *SUMA_Create_SO_map (void) 
{
   static char FuncName[]={"SUMA_Create_SO_map"};
   SUMA_SO_map *SOM = NULL;
   
   SUMA_ENTRY;
   
   SOM = (SUMA_SO_map *) SUMA_malloc (sizeof(SUMA_SO_map));
   if (!SOM) {
      fprintf (SUMA_STDERR, "Error %s: Failed to allocate for SOM.\n", FuncName);
      SUMA_RETURN (NULL);
   }
   
   SOM->N_Node = 0;
   SOM->NewNodeList = NULL;
   SOM->NodeVal = NULL;
   SOM->NodeDisp = NULL;
   SOM->NodeCol = NULL;
   
   SUMA_RETURN (SOM);
}

/*!
  function to free SO_map
*/
SUMA_Boolean SUMA_Free_SO_map (SUMA_SO_map *SOM) 
{
   static char FuncName[]={"SUMA_Free_SO_map"};
   
   SUMA_ENTRY;
   
   if (!SOM) {
      SUMA_RETURN (YUP);
   }

   if (SOM->NewNodeList) SUMA_free (SOM->NewNodeList);
   if (SOM->NodeVal) SUMA_free (SOM->NodeVal);
   if (SOM->NodeDisp) SUMA_free (SOM->NodeDisp);
   if (SOM->NodeCol) SUMA_free(SOM->NodeCol);
   
   SUMA_free (SOM);
   
   SUMA_RETURN (YUP);
}

/*!
  function to Show SO_map
*/
SUMA_Boolean SUMA_Show_SO_map (SUMA_SO_map *SOM, FILE *out) 
{
   static char FuncName[]={"SUMA_Show_SO_map"};
   int i=0, imax;
   
   SUMA_ENTRY;
   
   if (!out) out = SUMA_STDERR;
   
   fprintf (out, "\n%s: Showing contents of SUMA_SO_map structure:\n", FuncName); 
   if (!SOM) {
      fprintf (out, "\tpointer is NULL.\n");
      SUMA_RETURN (YUP);
   }
   
   if (SOM->N_Node > 5) imax = 5; 
   else imax = SOM->N_Node;
   
   fprintf (SUMA_STDERR, "NodeList, (1st %d elements):\n", imax);
   for (i=0; i<imax; ++i) {
      fprintf (SUMA_STDERR, "\t%f, %f, %f\n", 
               SOM->NewNodeList[3*i], SOM->NewNodeList[3*i+1],
               SOM->NewNodeList[3*i+2]);
   }

   SUMA_RETURN (YUP);
}


/*!
  function used to create a SUMA_MorphInfo structure
*/
SUMA_MorphInfo *SUMA_Create_MorphInfo (void) 
{
   static char FuncName[]={"SUMA_Create_MorphInfo"};
   SUMA_MorphInfo *MI = NULL;
   
   SUMA_ENTRY;
   
   MI = (SUMA_MorphInfo *) SUMA_malloc (sizeof(SUMA_MorphInfo));
   if (!MI) {
      fprintf (SUMA_STDERR, "Error %s: Failed to allocate for MI.\n", FuncName);
      SUMA_RETURN (NULL);
   }
   
   MI->IDcode = NULL;
   MI->N_Node_std = 0;
   MI->N_Node_orig = 0;
   MI->N_FaceSet_std = 0;
   MI->Weight = NULL;
   MI->ClsNodes = NULL;
   MI->FaceSetList = NULL;
   
   SUMA_RETURN (MI);
}

/*!
  function to free MorphInfo
*/
SUMA_Boolean SUMA_Free_MorphInfo (SUMA_MorphInfo *MI) 
{
   static char FuncName[]={"SUMA_Free_MorphInfo"};
   
   SUMA_ENTRY;

   if (!MI) {
      SUMA_RETURN (YUP);
   }

   if (MI->IDcode) SUMA_free (MI->IDcode);
   if (MI->Weight) SUMA_free (MI->Weight);
   if (MI->ClsNodes) SUMA_free (MI->ClsNodes);
   if (MI->FaceSetList) SUMA_free (MI->FaceSetList);
   
   SUMA_free (MI);
   
   SUMA_RETURN (YUP);
}


/*!
  SO_new = SUMA_morphToStd( SO, MI, nodeChk);

  Function to morph surface to standard grid.
  \param SO (SurfaceObject *) surface being morphed
  \param MI (SUMA_MorphInfo *) structure containing morph information
  \param nodeChk (SUMA_Boolean) checks that nodes indicated in MI for morphing actually exist in SO (possibly do not if SO is a patch); if nodeChk, SO->FN cannot be NULL 
  \ret SO_new (SUMA_SurfaceObject *) morphed surface; returned with NodeList, FaceSetList, N_Node, N_FaceSet, NodeDim, FaceSetDim, idcode_st

  Written by Brenna Argall
*/
SUMA_SurfaceObject* SUMA_morphToStd (SUMA_SurfaceObject *SO, SUMA_MorphInfo *MI, 
                                     SUMA_Boolean nodeChk) 
{
   static char FuncName[] = {"SUMA_morphToStd"};
   float *newNodeList = NULL;
   int *tmp_newFaceSetList = NULL, *newFaceSetList = NULL, *inclNodes=NULL;
   int i, j, N_FaceSet, ti;
   SUMA_SurfaceObject *SO_new=NULL;
   SUMA_Boolean LocalHead = NOPE;
   
   SUMA_ENTRY;

   SO_new = SUMA_Alloc_SurfObject_Struct(1);
   if (SO_new == NULL) {
      fprintf (SUMA_STDERR,
               "Error %s: Failed to allocate for Surface Object.", FuncName);
      SUMA_RETURN (NULL);
   }  

   newNodeList = (float *) SUMA_calloc( 3*MI->N_Node_std, sizeof(float));
   if (!newNodeList) {
      fprintf (SUMA_STDERR, "Error %s: Failed to allocate. \n", FuncName);
      SUMA_RETURN (NULL);
   }
   N_FaceSet = 0;
  
   if ( !nodeChk ) {
      /*assume all nodes contained in MI->ClsNodes to be 
         also in SO->FaceSetList*/
      fprintf(SUMA_STDERR, 
               "Warning %s: Assuming face sets of surface %s to contain\n"
               " all nodes indicated in morphing to standard mesh.\n\n", 
              FuncName, SO->State);
 
      for (i=0; i<(MI->N_Node_std); ++i){
         j = 3*i;

         newNodeList[j] = (MI->Weight[j])*SO->NodeList[3*(MI->ClsNodes[j])] +         //node0 x
            (MI->Weight[j+1])*SO->NodeList[3*(MI->ClsNodes[j+1])] +                   //node1 x
            (MI->Weight[j+2])*SO->NodeList[3*(MI->ClsNodes[j+2])];                    //node2 x
         newNodeList[j+1] = (MI->Weight[j])*SO->NodeList[3*(MI->ClsNodes[j])+1] +     //node0 y
            (MI->Weight[j+1])*SO->NodeList[3*(MI->ClsNodes[j+1])+1] +                 //node1 y
            (MI->Weight[j+2])*SO->NodeList[3*(MI->ClsNodes[j+2])+1];                  //node2 y
         newNodeList[j+2] = (MI->Weight[j])*SO->NodeList[3*(MI->ClsNodes[j])+2] +     //node0 z
            (MI->Weight[j+1])*SO->NodeList[3*(MI->ClsNodes[j+1])+2] +                 //node1 z
            (MI->Weight[j+2])*SO->NodeList[3*(MI->ClsNodes[j+2])+2];                  //node2 z   
      }

      newFaceSetList = MI->FaceSetList;
      N_FaceSet = MI->N_FaceSet_std;
   }

   else {
      /*check MI->ClsNodes for possibly containing nodes which are not included in SO->FaceSetList*/

      if ( !SO->FN ) {
         fprintf( SUMA_STDERR, 
                  "Error %s: No First Neighbor information passed.\n", FuncName);
         SUMA_RETURN (NULL);
      }

      /*keep track of included MI nodes; 1=>included, 0=>not*/
      inclNodes = SUMA_calloc( MI->N_Node_std, sizeof(int));
      for (i=0; i<MI->N_Node_std; ++i) {
         inclNodes[i] = 0;
      }

      for (i=0; i<(MI->N_Node_std); ++i) {
         
         j = 3*i;
         if (  (MI->ClsNodes[j])  <(SO->N_Node) && 
               (MI->ClsNodes[j+1])<(SO->N_Node) &&  
               (MI->ClsNodes[j+2])<(SO->N_Node) ) {  
                        /* CONDITIONS USED TO BE <= ; NOT GOOD    ZSS FEB 07 */
                        /* index of 3 nodes in MI->ClsNodes do not exceed 
                           number of nodes in SO*/
            if (  SO->FN->N_Neighb[MI->ClsNodes[j]]>0 && 
                  SO->FN->N_Neighb[MI->ClsNodes[j+1]]>0 && 
                  SO->FN->N_Neighb[MI->ClsNodes[j+2]]>0 ) {
                     /* 3 nodes in MI->ClsNodes are all a part of the SO mesh 
                        (have at least 1 neighbor in the SO mesh)*/
                     /* Make sure these three nodes do form a valid facet. 
                        Otherwise, whine. */
               if ( (ti = SUMA_whichTri(  SO->EL, MI->ClsNodes[j], 
                                          MI->ClsNodes[j+1], MI->ClsNodes[j+2], 
                                          0, 0)) < 0) {
                  SUMA_S_Warnv ( "Node %d of the mapping structure has\n"
                                 " three closest nodes %d %d %d that do\n"
                                 " not form a triangle in %s's mesh.\n", 
                                 i, MI->ClsNodes[j], MI->ClsNodes[j+1], 
                                 MI->ClsNodes[j+2], SO->Label);
                                  
               }  

               inclNodes[i]   = 1; 
               newNodeList[j]   = (MI->Weight[j])*SO->NodeList[3*(MI->ClsNodes[j])] +       //node0 x
                  (MI->Weight[j+1])*SO->NodeList[3*(MI->ClsNodes[j+1])] +                   //node1 x
                  (MI->Weight[j+2])*SO->NodeList[3*(MI->ClsNodes[j+2])];                    //node2 x
               newNodeList[j+1] = (MI->Weight[j])*SO->NodeList[3*(MI->ClsNodes[j])+1] +     //node0 y
                  (MI->Weight[j+1])*SO->NodeList[3*(MI->ClsNodes[j+1])+1] +                 //node1 y
                  (MI->Weight[j+2])*SO->NodeList[3*(MI->ClsNodes[j+2])+1];                  //node2 y
               newNodeList[j+2] = (MI->Weight[j])*SO->NodeList[3*(MI->ClsNodes[j])+2] +     //node0 z
                  (MI->Weight[j+1])*SO->NodeList[3*(MI->ClsNodes[j+1])+2] +                 //node1 z
                  (MI->Weight[j+2])*SO->NodeList[3*(MI->ClsNodes[j+2])+2];                  //node2 z   
            }
         } else {
           /*otherwise, morphing for this node skipped*/
           SUMA_LHv(    "MI->ClsNodes[%d || %d || %d] = SO->N_Node=%d\n", 
                        j, j+1, j+2, SO->N_Node);
         }
      }

      /*create list of MI facesets for which all 3 nodes were morphed*/
      tmp_newFaceSetList = SUMA_calloc( 3*MI->N_FaceSet_std, sizeof(int));
      if (!tmp_newFaceSetList) {
         fprintf (SUMA_STDERR, "Error %s: Failed to allocate. \n", FuncName);
         SUMA_RETURN (NULL);
      }

      for (i=0; i<MI->N_FaceSet_std; ++i) {
         j = 3*i;
         if (  inclNodes[MI->FaceSetList[j]]==1 && 
               inclNodes[MI->FaceSetList[j+1]]==1 && 
               inclNodes[MI->FaceSetList[j+2]]==1) {
            /*all nodes morphed for this faceset*/
            tmp_newFaceSetList[3*N_FaceSet]   = MI->FaceSetList[j];
            tmp_newFaceSetList[3*N_FaceSet+1] = MI->FaceSetList[j+1];
            tmp_newFaceSetList[3*N_FaceSet+2] = MI->FaceSetList[j+2];
            N_FaceSet++;
         }
      }

      /*create final new face list of correct length*/
      if ( N_FaceSet == MI->N_FaceSet_std ) {
         /*all facesets in MI->FaceSetList included*/
         newFaceSetList = tmp_newFaceSetList;
      }
      else {
         /*some facesets in MI->FaceSetList not included*/
         newFaceSetList = SUMA_calloc( 3*N_FaceSet, sizeof(int));
         if (!newFaceSetList) {
            fprintf (SUMA_STDERR, "Error %s: Failed to allocate. \n", FuncName);
            SUMA_RETURN (NULL);
         }
         for (i=0; i<3*N_FaceSet; ++i) {
            newFaceSetList[i] = tmp_newFaceSetList[i];
         }
         SUMA_free (tmp_newFaceSetList);
      }
      SUMA_free (inclNodes);
   }
    
   /* store in SO_new and get out */
   SO_new->NodeList = newNodeList;
   SO_new->FaceSetList = newFaceSetList;
   SO_new->N_Node = MI->N_Node_std;
   SO_new->N_FaceSet = N_FaceSet;
   SO_new->NodeDim = 3;
   SO_new->FaceSetDim = 3;
   SO_new->idcode_str = (char *)SUMA_calloc (SUMA_IDCODE_LENGTH, sizeof(char));   
   UNIQ_idcode_fill (SO_new->idcode_str);

   SUMA_RETURN( SO_new );
}

/*!
  array = SUMA_readColor( numNodes, colFileNm);

  Function to read a colorfile into an array.
  \param numNodes (int) size of created array
  \param colFileNm (char *) name of color file to be read
  \ret colArray (float *) array of colorfile values

  Written by Brenna Argall
*/
float* SUMA_readColor (int numNodes, char* colFileNm) {

   float *colArray=NULL;
   FILE *colFile=NULL;
   char *line=NULL, *temp=NULL;
   int i=0, j=0, k=0, index=0;
   static char FuncName[]={"SUMA_readColor"};
   
   SUMA_ENTRY;
   
   colArray = (float *) SUMA_calloc( 3*numNodes, sizeof(float) );
   line = (char *) SUMA_calloc( 10000, sizeof(char));
   temp = (char *) SUMA_calloc( 10000, sizeof(char));

   if( (colFile = fopen(colFileNm, "r"))==NULL) {
      fprintf (SUMA_STDERR, "Failed in opening %s for reading.\n", colFileNm);
      if (colArray) SUMA_free (colArray);
      if (line) SUMA_free (line);
      if (temp) SUMA_free (temp);
      exit(1);
   }
   else {
      fgets( line, 1000, colFile);
      while( !feof(colFile) ) {

         j = 3*index;
         i = 0;
         while ( isdigit(line[i]) ) ++i;
     
         ++i;  k=0;
         while ( !isspace(line[i])) {
            temp[k] = line[i];
            ++i;  ++k;
         }
         colArray[j] = atof(temp);
         SUMA_free(temp);
         temp = SUMA_calloc(10000, sizeof(char));
      
         ++i;  k=0;
         while ( !isspace(line[i])) {
            temp[k] = line[i];
            ++i;  ++k;
         }
         colArray[j+1] = atof(temp);
         SUMA_free(temp);
         temp = SUMA_calloc( 10000, sizeof(char));
      
         ++i;  k=0;
         while ( !isspace(line[i])) {
            temp[k] = line[i];
            ++i;  ++k;
         }
         colArray[j+2] = atof(temp);
         SUMA_free(temp);
         temp = SUMA_calloc( 10000, sizeof(char));
      
         fgets( line, 10000, colFile ); 
         ++index;
      }
   }
   SUMA_free(line);
   SUMA_free(temp);

   SUMA_RETURN( colArray);
}

/*!
  SUMA_writeColorFile(array, size, fileNm);

  Function to write out colorfile.
  \param array (float*) list of colors to be written
  \param numNode (int) number of nodes to be xwritten to file
  \param index (int*) array of node indices to receive color; 
         pass as NULL if index standard (increments by one) 
  \param fileNm (char) name of file to be written to

  Written by Brenna Argall
*/
void SUMA_writeColorFile (float *array, int numNode, int *index, char fileNm[]) {   

   FILE *outFile=NULL;
   int i=0, j=0;
   static char FuncName[] = {"SUMA_writeColorFile"};
   
   SUMA_ENTRY;
   
   for (i=0; i<numNode; ++i) {
      j = 3*i;
   }

   for (i=0; i<numNode; ++i) {
      j = 3*i;
   }

   if((outFile = fopen(fileNm, "w"))==NULL) {
      fprintf(SUMA_STDERR, "Could not open file %s.\n", fileNm);
      exit(1);
   }
   else {
      if (index!=NULL) {
         /*use given indices*/
         for (i=0; i<numNode; ++i) {
            j = 3*i;
            fprintf (outFile, "%d\t%f\t%f\t%f\n", 
                     index[i], array[j], array[j+1], array[j+2]);
         }
      }
      else {
         /*assume indices increment by 1 (all nodes have color)*/
         for (i=0; i < numNode; ++i) {
            j = i*3;
            fprintf (outFile, "%d\t%f\t%f\t%f\n", 
                     i, array[j], array[j+1], array[j+2]);
         }
      }
      fclose (outFile);
   }
   SUMA_RETURNe;
}

/*!
  SUMA_writeFSfile(SO, firstLine, fileNm);

  Function to write out file in freesurfer format. 
  \param nodeList (float *) list of nodes
  \param faceList (int *) list of faces
  \param numNode (int) number of nodes
  \param numFace (int) number of faces
  \param firstLine (char) comment string for first line of file
  \param fileNm (char) name of file to be written to
  \ret void

  Written by Brenna Argall
*/
void SUMA_writeFSfile (SUMA_SurfaceObject *SO, char firstLine[], char fileNm[]) {

   FILE *outFile=NULL;
   int i=0, j=0;
   static char FuncName[]={"SUMA_writeFSfile"};
  
   SUMA_ENTRY; 
  
   outFile = fopen(fileNm, "w");
   if (!outFile) {
      fprintf (SUMA_STDERR, 
               "Error %s: Failed in opening %s for writing.\n",FuncName, fileNm);
      exit(1);
   }
   else {
      if ( firstLine!=NULL ) 
         fprintf (outFile,"#%s\n", firstLine);
      else fprintf (outFile, "#!ascii version of FreeSurfer surface\n");
      fprintf (outFile, "%d %d\n", SO->N_Node, SO->N_FaceSet);
    
      j=0;
      for (i=0; i<SO->N_Node; ++i) {
         j=3*i;
         fprintf (outFile, "%f  %f  %f  0\n", 
                  SO->NodeList[j], SO->NodeList[j+1], SO->NodeList[j+2]);
      }
    
      j=0;
      for (i=0; i<SO->N_FaceSet; ++i) {
         j = 3*i;
         fprintf (outFile, "%d %d %d 0\n", 
                  SO->FaceSetList[j], SO->FaceSetList[j+1], 
                  SO->FaceSetList[j+2]);
      }
    
      fclose(outFile);
   }
  
   SUMA_RETURNe;
}

#if 0
/*!
  SUMA_writeSpecFile( surfaces, numSurf, program, group, specFileNm, char *histnote);

  Function to write suma spec file.
  \param surfaces (SUMA_specSurfInfo *) necessary surface information for spec file
  \param numSurf (int) number of surfaces in spec file
  \param program (char[]) name of program calling function
  \param group (char[]) name of group
  \param fileNm (char[]) name of created spec file
  \return void

  Written by Brenna Argall
*/

void SUMA_writeSpecFile_old (  SUMA_SpecSurfInfo *surfaces, int numSurf, 
                           char program[], char group[], char specFileNm[], 
                           char *histnote) {

   FILE *outFile=NULL;
   int i=0, k=0, tag=0, ifSmwm=0, p=0;
   static char FuncName[]={"SUMA_writeSpecFile_old"};
      
   SUMA_ENTRY;

   outFile = fopen(specFileNm, "w");
   if (!outFile) {
      fprintf (SUMA_STDERR, "Failed in opening %s for writing.\n", specFileNm); 
      exit (1);
   }
   else {
      fprintf (outFile, "# %s spec file for %s\n", program, group);
      if (histnote) fprintf (outFile, "#History: %s\n\n", histnote);
      else fprintf (outFile, "\n");
      fprintf (outFile, "#define the group\n\tGroup = %s\n\n", group);
      fprintf (outFile, "#define various States\n");
      for (i=0; i<numSurf; ++i) {
         tag = 0;
         for (k=0; k<i; ++k) {
            if ( strcmp( surfaces[k].state, surfaces[i].state ) == 0) tag = -1;
         }
         if (tag==0) {
            fprintf( outFile, "\tStateDef = %s\n", surfaces[i].state);
         }
      }

      for (i=0; i<numSurf; ++i) {
         fprintf (outFile, 
                  "\nNewSurface\n\tSurfaceFormat = %s\n\tSurfaceType = %s\n", 
                  surfaces[i].format, surfaces[i].type);
         fprintf (outFile, 
                  "\tFreeSurferSurface = %s\n\tLocalDomainParent = %s\n", 
                  surfaces[i].fileToRead, surfaces[i].mapRef );
         fprintf (outFile, "\tSurfaceState = %s\n\tEmbedDimension = %s\n", 
                  surfaces[i].state, surfaces[i].dim);
      }

      fclose(outFile);
   }
   SUMA_RETURNe;
}
#endif

/*!
   OBSOLETE: Use SUMA_Write_SpecFile
   
  SUMA_writeSpecFile( surfaces, numSurf, program, group, specFileNm, char *histnote);

  Function to write suma spec file.
  \param surfaces (SUMA_specSurfInfo *) necessary surface information for spec file
  \param numSurf (int) number of surfaces in spec file
  \param program (char[]) name of program calling function
  \param group (char[]) name of group
  \param fileNm (char[]) name of created spec file
  \return void

  Written by Brenna Argall
*/
void SUMA_writeSpecFile (SUMA_SpecSurfInfo *surfaces, int numSurf, char program[], char group[], char specFileNm[], char *histnote) {

   FILE *outFile=NULL;
   int i=0, k=0, tag=0, ifSmwm=0, p=0;
   static char FuncName[]={"SUMA_writeSpecFile"};
      
   SUMA_ENTRY;

/*
   OBSOLETE: Use SUMA_Write_SpecFile
*/   

   outFile = fopen(specFileNm, "w");
   if (!outFile) {
      fprintf (SUMA_STDERR, "Failed in opening %s for writing.\n", specFileNm); 
      exit (1);
   }
   else {
      fprintf (outFile, "# %s spec file for %s\n", program, group);
      if (histnote) fprintf (outFile, "#History: %s\n\n", histnote);
      else fprintf (outFile, "\n");
      fprintf (outFile, "#define the group\n\tGroup = %s\n\n", group);
      fprintf (outFile, "#define various States\n");
      for (i=0; i<numSurf; ++i) {
         tag = 0;
         for (k=0; k<i; ++k) {
            if ( strcmp( surfaces[k].state, surfaces[i].state ) == 0) tag = -1;
         }
         if (tag==0) {
            fprintf( outFile, "\tStateDef = %s\n", surfaces[i].state);
         }
      }

      for (i=0; i<numSurf; ++i) {
         fprintf (outFile, "\nNewSurface\n\tSurfaceFormat = %s\n\tSurfaceType = %s\n", surfaces[i].format, surfaces[i].type);
         fprintf (outFile, "\tFreeSurferSurface = %s\n\tLocalDomainParent = %s\n", surfaces[i].fileToRead, surfaces[i].mapRef );
         fprintf (outFile, "\tSurfaceState = %s\n\tEmbedDimension = %s\n", surfaces[i].state, surfaces[i].dim);
      }

      fclose(outFile);
   }
   SUMA_RETURNe;
}
   


/*!
  void = SUMA_readANOVA1D( fileNm, i_colm, i_locInfo, data );

  Function to read a 1D file into an array.
  \param fileNm (char *) name of 1D file to be read
  \param i_colm (int *) indicates which value columns of 1D file to be read; [0] should contain the number of columns to be read
  \param i_locInfo (int *) gives column index of location information; [0] for node index, [1] for voxel index, [2],[3],[4] for ijk values; pass as NULL if file contains none of this information, or with -1 for the absence of a particular column
  \param data (SUMA_1dData *) structure passed back containing file information (can be passed empty, must be allocated for); multiple value columns are passed back concatanated as a 1D array in 'valArray' 
  \ret A (SUMA_1dData *) structure containing column information indicated

  Written by Brenna Argall
*/
void SUMA_read1D (char* fileNm, int* i_colm, int* i_locInfo, SUMA_1dData* data) {

   FILE *file=NULL;
   char *line=NULL, *frag=NULL;
   char scan[100];
   int i=0, j=0, k=0, num_node=0, lgth=0, i_curr=0, i_last=0; 
   int num_loc=0, num_val=0, num_tot=0, valCnt=0, tempInt=0;
   int *i_colm_ndx=NULL, *i_colmSrtd=NULL, *i_cat=NULL;
   float *valArray=NULL, tempFlt=0;
   int *ndx_list=NULL, *vxl_list=NULL, *ijk_list=NULL, *nvox_list=NULL;
   SUMA_Boolean nd_given=NOPE;
   static char FuncName[]={"SUMA_read1D"};

   SUMA_ENTRY;

   /**set default length to 500,000*/
   lgth = 500000;
   
   /*find number of location indices*/
   if (i_colm[0] == 0) {
      fprintf(SUMA_STDERR, "\nError %s: No column indices given! Exiting.\n", FuncName);
      exit(1);
   }
   else  num_tot = i_colm[0];
   num_loc=0;
   /**determine number of location columns and value columns to be read*/
   for (i=0; i<6; ++i) {
      for (j=0; j<num_tot-1; ++j) {
         if ( i_locInfo[i]==i_colm[j+1] ) {
            /*indicates location column*/
            ++num_loc;
         }
      }
   }
   
   num_val = num_tot-num_loc;

   /*sort column indices*/
   i_colmSrtd = SUMA_calloc( i_colm[0]-1, sizeof(int));
   for (i=0; i<i_colm[0]; ++i) {
      /*copy i_colm removing first element (contains number of columns)*/
      i_colmSrtd[i] = i_colm[i+1];
   }
   i_colm_ndx = SUMA_z_dqsort( i_colmSrtd, num_tot );

   /*keep track of which columns are node, voxel, ijk, nvox and value*/
   i_cat = SUMA_calloc( num_tot, sizeof(int));
   for ( i=0; i<num_tot; ++i) {
      if ( i_colmSrtd[i]==i_locInfo[0] ) {                   // 0=>node index column
         i_cat[i] = 0;      
         nd_given = YUP;
      }
      else if ( i_colmSrtd[i]==i_locInfo[1] ) i_cat[i] = 1;  // 1=>voxel index column
      else if ( i_colmSrtd[i]==i_locInfo[2] ) i_cat[i] = 2;  // 2=>i location column
      else if ( i_colmSrtd[i]==i_locInfo[3] ) i_cat[i] = 3;  // 3=>j location column
      else if ( i_colmSrtd[i]==i_locInfo[4] ) i_cat[i] = 4;  // 4=>k location column
      else if ( i_colmSrtd[i]==i_locInfo[5] ) i_cat[i] = 5;  // 5=>nvox column
      else                                    i_cat[i] = -1; //-1=> value column
   }

   valArray = SUMA_calloc( num_val*lgth, sizeof(float) );
   ndx_list = SUMA_calloc( lgth, sizeof(int) );
   vxl_list = SUMA_calloc( lgth, sizeof(int) );
   ijk_list = SUMA_calloc( 3*lgth, sizeof(int) );
   nvox_list = SUMA_calloc( lgth, sizeof(int) );

   line = SUMA_calloc( 10000, sizeof(char));
   
   if ( !valArray || !ndx_list || !vxl_list || !ijk_list || !nvox_list || !line) {
      fprintf(SUMA_STDERR, "Error %s: Failed in allocation.\n", FuncName);
      if (valArray)  SUMA_free(valArray);
      if (ndx_list)  SUMA_free(ndx_list);
      if (vxl_list)  SUMA_free(vxl_list);
      if (ijk_list)  SUMA_free(ijk_list);
      if (nvox_list) SUMA_free(nvox_list);
      if (line)      SUMA_free(line);
      exit(1);
   }

   if( (file = fopen(fileNm, "r"))==NULL) {
      fprintf (SUMA_STDERR, "Failed in opening %s for reading.\n", fileNm);
      if (valArray)  SUMA_free(valArray);
      if (ndx_list)  SUMA_free(ndx_list);
      if (vxl_list)  SUMA_free(vxl_list);
      if (ijk_list)  SUMA_free(ijk_list);
      if (nvox_list) SUMA_free(nvox_list);
      if (line)      SUMA_free(line);
      exit(1);
   }
   
   else {
      
      /**skip through comments*/
      fgets( line, 1000, file);
      while( line[0]=='#' ) {
         fgets( line, 10000, file);
      }
      
      /**read remaining values*/
      num_node = 0;
      while( !feof(file) && line[0]!='#' && num_node<lgth) {
         valCnt = 0;
         i_last = 0;
         frag = strtok(line, " \t\n\r");
         if (frag==NULL) {
            fprintf(SUMA_STDERR, 
                 "\nError %s: Indicated column for file not found. Exiting.\n", 
                 FuncName);
            exit(1);
         }

         for ( k=0; k<num_tot; ++k ) {
            for ( i=0; i<i_colmSrtd[k]-i_last; ++i) {
               frag = strtok(NULL, " \t\n\r"); 
               if (frag==NULL) {
                  fprintf(SUMA_STDERR, 
            "\nError %s: Indicated column for file not found. Exiting.\n", 
                  FuncName);
                  exit(1);
               }
            }
            
            if (frag==NULL) {
               fprintf(SUMA_STDERR, 
               "\nError %s: Indicated column for file not found. Exiting.\n", 
                  FuncName);
               exit(1);
            }

            if ( i_cat[k]!=-1 ) {
               /*look for int (location column)*/
               sscanf(frag, "%d", &tempInt);
            }
            else {
               /*look for float (value column*/
               sscanf(frag, "%f", &tempFlt);
            }
            
            if ( i_cat[k]==0 )      ndx_list[num_node] = tempInt;      // node
            else if ( i_cat[k]==1 ) vxl_list[num_node] = tempInt;      // voxel
            else if ( i_cat[k]==2 ) ijk_list[3*num_node] = tempInt;    // i
            else if ( i_cat[k]==3 ) ijk_list[3*num_node+1] = tempInt;  // j
            else if ( i_cat[k]==4 ) ijk_list[3*num_node+2] = tempInt;  // k
            else if ( i_cat[k]==5 ) nvox_list[num_node] = tempInt;     // nvox
            else valArray[ (valCnt++)*lgth + num_node ] = tempFlt;     // value
            i_last = i_colmSrtd[k];
         }
         fgets( line, 10000, file);
         ++num_node;
      }  
      fclose(file);
   }
   
   
   /**create array of exact length to pass back*/
   data->N_elem = num_node;
   data->nd_list = SUMA_calloc( num_node, sizeof(int));
   data->vxl_list = SUMA_calloc( num_node, sizeof(int));
   data->ijk_list = SUMA_calloc( 3*num_node, sizeof(int));
   data->nvox_list = SUMA_calloc( num_node, sizeof(int));
   data->valArray = SUMA_calloc( num_val*num_node, sizeof(float));

   for (i=0; i<num_node; ++i) {
      if (nd_given) data->nd_list[i] = ndx_list[i];
      else data->nd_list[i] = i;
      data->vxl_list[i] = vxl_list[i];
      data->ijk_list[i] = ijk_list[i];
      data->nvox_list[i] = nvox_list[i];
      for (k=0; k<num_val; ++k) {
         data->valArray[ k*num_node +i ] = valArray[ k*lgth +i ];
      }
   }

   SUMA_free(line);
   SUMA_free(i_colm_ndx);
   SUMA_free(i_colmSrtd);
   SUMA_free(i_cat);

   SUMA_free(valArray);
   SUMA_free(ndx_list);
   SUMA_free(vxl_list);
   SUMA_free(ijk_list);
   SUMA_free(nvox_list);

   SUMA_RETURNe;
}


/*!
  SUMA_write1D( num, vals, outFileNm);

  Function to write simple 1D file.
  \param num (int*) [0] contains number of rows, [1] number of columns, to be written
  \param vals (float*) vector of values (size: num[0]*num[1], format: contcatanated rows)
  \param index (int*) vector of indices (size: num[0]); pass as NULL if standard increment
  \param firstline (char[]) comment for top of file
  \param outFileNm (char[]) name of file written to
  \return void

  Written by Brenna Argall
*/
void SUMA_write1D ( int *num, float *vals, int *index, char firstline[], char outFileNm[]) {

   FILE *outFile=NULL;
   int i=0, j=0, k=0;
   static char FuncName[]={"SUMA_write1D"};
      
   SUMA_ENTRY;

   outFile = fopen(outFileNm, "w");
   if (!outFile) {
      fprintf (SUMA_STDERR, "Failed in opening %s for writing.\n", outFileNm); 
      exit (1);
   }
   else {
      if (firstline!=NULL) fprintf (outFile, "%s\n", firstline);
      for (i=0; i<num[0]; ++i) {
         if ( index!=NULL ) {
            /*index array given - use only those indices from vals*/
            j = index[i] * num[1];
            fprintf( outFile, "%10d   ", index[i]);
         }
         else j = i*num[1];  /*index array not given - standard increment*/
         for ( k=0; k<num[1]; ++k ) {
            /*print vals to file*/
            fprintf( outFile, "%10f   ", vals[ j+k ]);
         }
         fprintf( outFile, "\n");
      }
      fclose(outFile);
   }
   SUMA_RETURNe;
}


/*!
  array = SUMA_createColGradient( col, numDiv);

  Function to create a color gradient.
  \param col (float *) vector of colors for range (1->8, roygbivr). If allGvn==YUP, pass only two (which are taken as endpoints). If NULL, assumed to be continuous
  \param numSeg (int) number of segments in final gradient (=length of col if allGvn==YUP); pass as -1 for continuous gradient
  \param addGvn (SUMA_Boolean) indicates whether col expressly gives all colors to be used in colSeg
  \ret colSeg (float *) vector of numSeg (or 700, if numSeg==-1) colors, in 3x(numSeg) format (corresponding to RGB)

  Written by Brenna Argall
*/

float * SUMA_createColGradient( float *col, int numSeg, SUMA_Boolean allGvn ) {

   int i, j, k, it;
   int numCol=0, numStdIncr, numColGvn;
   int *colRngInt=NULL, *colUsed=NULL, i_incr=0;
   int *bind_currCol=NULL, *distTOint=NULL, colIntArray[8];
   int dist_intTOrngBeg, dist_intTOrngEnd, *numColDiv=NULL, *tmpInt=NULL;
   float *colRng=NULL, color[8][3], *colSeg=NULL, *colIncr=NULL, *stdColIncr=NULL;
   float incR=0.0, incG=0.0, incB=0.0, temp, dist[2];
   SUMA_Boolean decr = NOPE, noGrad = NOPE;
   static char FuncName[]={"SUMA_createColGradient"};

   SUMA_ENTRY;
   fprintf(SUMA_STDERR, "numSeg = %d\n", numSeg);
   if ( (col==NULL || numSeg<0) && allGvn) {
      /*should be mutually exclusive => assume meant !allGvn and use entire spectrum
        (claims all colors expressly given, yet none are passed or claims continuous gradient)*/
      allGvn = NOPE;
   }

   /**determine color range*/

   if (col==NULL) {
      /*assume full color range*/
      colRngInt = SUMA_calloc(2, sizeof(int));
      colRng = SUMA_calloc(2, sizeof(int));
      colRng[0] = 0;
      colRng[1] = 7;
/*      colRngInt[0] = 0;
      colRngInt[1] = 0;
      colRngInt[2] = 7;
      colRngInt[3] = 7;*/
   }      
   else {
      if ( col[0]<0 || col[1]>7 || col[0]<0 || col[1]>7) {
         fprintf(SUMA_STDERR, "\nError %s: Color Ranges not within 0->7. Exiting.\n", FuncName);
         exit(1);
      }
      
      /*take passed color range*/
      if ( allGvn ) {
         /*assign each color*/
         colRng = SUMA_calloc(numSeg, sizeof(float));
         for (i=0; i<numSeg; ++i) {
            colRng[i] = col[i]; }
      }
      else {
         /*assign only endpoints*/
         colRng = SUMA_calloc(2, sizeof(float));
         colRng[0] = col[0];
         colRng[1] = col[1];
      
         /**check decreasing or increasing color numbers (does not matter if all colors given)*/
         if ( col[1] < col[0] ) {  
            decr = YUP;
         }
      }
   }
   
   /**roygbiv values for each color*/
   color[0][0]=.75;   color[0][1]=0;    color[0][2]=0;   //red
   color[1][0]=1;   color[1][1]=.5;   color[1][2]=0;   //orange
   color[2][0]=1;   color[2][1]=1;    color[2][2]=0;   //yellow
   color[3][0]=0;   color[3][1]=.75;  color[3][2]=0;   //green
   color[4][0]=0;   color[4][1]=.75;   color[4][2]=.75;   //blue
   color[5][0]=0;   color[5][1]=0;    color[5][2]=.75;   //indigo
   color[6][0]=.5;  color[6][1]=0;    color[6][2]=.75;   //violet
   color[7][0]=.75;   color[7][1]=0;    color[7][2]=0;   //red
   
   /*determine number of segments (to divide stdIncrements)*/
   if (numSeg<0) numSeg = 700;  /*continuous gradient*/
   
   
   /*create array of 0.1 and integer increments*/
   stdColIncr = SUMA_calloc( 71, sizeof(float));
   stdColIncr[0] = 0.0;
   colIntArray[0] = 0;
   for ( i=0; i<7; ++i) {
      colIntArray[i+1] = i+1;
      for ( j=1; j<11; ++j) {
         stdColIncr[ 10*i+j ] = stdColIncr[ 10*i ] + j*0.1;
      }
      /*elaborate because just adding .1 was summing to less than 7 (likely float truncation issue)*/
   }
   

   /**find stdColIncr and Integers affiliated with given colors*/

   if (allGvn) numColGvn = numSeg;
   else numColGvn = 2;
   if (!colRngInt)
      colRngInt = SUMA_calloc( 2*numColGvn, sizeof(int));
   bind_currCol = SUMA_calloc( 2, sizeof(int));
   distTOint = SUMA_calloc( numColGvn, sizeof(float));

   for (i=0; i<numColGvn; ++i) {
      
      /*find stdColIncr elements nearest to given colors; set given colors to stdColIncr*/
      bind_currCol[0] = 0;  bind_currCol[1] = 70;
      if ( !SUMA_binSearch( stdColIncr, colRng[i], bind_currCol, 0 ) ) {
         fprintf(SUMA_STDERR, "\nError %s: Failed in binary search !(%f < %f < %f). Exiting.\n\n", FuncName, stdColIncr[bind_currCol[0]], colRng[i], stdColIncr[bind_currCol[1]]);
         if (colRng) SUMA_free(colRng);
         if (stdColIncr) SUMA_free(stdColIncr);
         if (colRngInt) SUMA_free(colRngInt);
         if (bind_currCol) SUMA_free(bind_currCol);
         if (distTOint) SUMA_free(distTOint);
         exit(1);
      }
      if ( abs( stdColIncr[bind_currCol[0]]-colRng[i] ) < abs( stdColIncr[bind_currCol[1]]-colRng[i] )) 
         colRng[i] = stdColIncr[bind_currCol[0]];
      else colRng[i] = stdColIncr[bind_currCol[1]];
      

      /*find integers binding new color*/
      /*(note integers passed as floats merely truncated to ints)*/
      if ( abs( colRng[i] - (int)colRng[i] )<0.01 ) {
         /*(assume colRng[i] passed as integer in float format)*/ 
         colRngInt[ 2*i ] = (int)colRng[i];
         colRngInt[ 2*i+1 ]  = (int)colRng[i];
      }
      else {
         colRngInt[ 2*i ] = (int)colRng[i];
         colRngInt[ 2*i+1 ] = (int)colRng[i]+1;
      }
      
      /*find distance from color to low binding integer (high binding is 10-(dist to low))*/
      distTOint[i] = bind_currCol[0]%10;

      fprintf(SUMA_STDERR, "%d: %d < %f < %d     dist = %d\n", i, colRngInt[2*i], colRng[i], colRngInt[2*i+1], distTOint[i]);
   }


   /**divide range*/

   if ( !allGvn ) {
      /*color range end point given - must divide range*/

      if ( !decr ) numCol = colRngInt[3]-colRngInt[0] + 1;
      else numCol = colRngInt[1] - colRngInt[2] + 1;
      
      /*determine all colors within range*/
      colUsed = SUMA_calloc(numCol, sizeof(int));
      colUsed[0] = colRngInt[0];
      for (i=1; i<numCol; ++i) {
         if ( !decr ) colUsed[i] = colUsed[i-1] + 1;
         else colUsed[i] = colUsed[i-1] - 1;
      }

      /*determine distance from colRng to binding integers*/ 
      if ( !decr ) { 
         dist_intTOrngBeg = distTOint[0];
         dist_intTOrngEnd = 10-distTOint[1];
      }
      else {
         dist_intTOrngBeg = 10-distTOint[0];
         dist_intTOrngEnd = distTOint[1];
      }
      
      /*determine the number of increments necessary for color (based upon 10 gradations between each color)
        to be later (further) divided by the number of segments needed for this specific gradient*/
      /*numStdIncr is number of std (.1) gradations included in colRng*/
      if (numCol>3) 
         /*color blocks exist between the first and last*/
         numStdIncr = 10*(numCol-1) - dist_intTOrngBeg - dist_intTOrngEnd;
      else if ( numCol==3 ) {
         /*first and last color blocks distinct but with no color blocks between*/
         numStdIncr = 20 - dist_intTOrngBeg - dist_intTOrngEnd;
      }
      else {
         /*first and last color blocks the same (colRng found within only one color block)*/
         numStdIncr = 10 - dist_intTOrngBeg - dist_intTOrngEnd;
         noGrad = YUP;
      }
      
      if ( noGrad ) {
         /*start and end colors the same => no subdivisions necessary
        (an entirely redundant input if numSeg>1 since no divisions will even be seen)*/
         
         for (i=0; i<numSeg; ++i ) {
            k = 3*i;
            colSeg[k]   = color[colRngInt[0]][0];
            colSeg[k+1] = color[colRngInt[0]][1];
            colSeg[k+2] = color[colRngInt[0]][2];
         }
      }
      
      else {
         /*subdivide colors*/
         
         /*create array containing number of .1 divisions within colRng for each color block*/
         numColDiv = SUMA_calloc( numCol-1, sizeof(int));
         numColDiv[0] = 10 - dist_intTOrngBeg;               //first color block
         for (i=1; i<numCol-2; ++i) { numColDiv[i] = 10; }   //middle color blocks
         numColDiv[numCol-2] = 10 - dist_intTOrngEnd;        //last color block
         
         /**create array of small increments to fractionize incremental colors for segments*/
         /* (see LNB pg ??)*/
      
         colIncr = SUMA_calloc( 3*(numSeg-1)*(numStdIncr), sizeof(float));

         k=0;
         for (i=0; i<numCol-1; ++i) {
            
            /*divide current color by the number of incremental segments*/
            incR = (color[colUsed[i+1]][0] - color[colUsed[i]][0])/(10*(numSeg-1));
            incG = (color[colUsed[i+1]][1] - color[colUsed[i]][1])/(10*(numSeg-1));
            incB = (color[colUsed[i+1]][2] - color[colUsed[i]][2])/(10*(numSeg-1));
            
            /*place (numColDiv[i]*(numSeg-1)) color increments into colIncr array*/ 
            for (j=0; j<numColDiv[i]*(numSeg-1); ++j) {
               colIncr[k++] = incR;
               colIncr[k++] = incG;
               colIncr[k++] = incB;
            }
         }
      }
      
      
      /**assign segment colors by summing increments*/
      
      colSeg = SUMA_calloc( 3*numSeg, sizeof(float));
      
      colSeg[0] = color[colUsed[0]][0];
      colSeg[1] = color[colUsed[0]][1];
      colSeg[2] = color[colUsed[0]][2];
      for (i=0; i<dist_intTOrngBeg; ++i) {
         /*first colSeg must jump to fractional (.1) color*/
         /*note that the first three elements (rgb) of colIncr are always increments of the first color block
           so taking just those three repeatedly is fine*/
         colSeg[0] = colSeg[0] + colIncr[0];
         colSeg[1] = colSeg[1] + colIncr[1];
         colSeg[2] = colSeg[2] + colIncr[2];
      }
      
      i_incr = 0; 
      fprintf(SUMA_STDERR, "numSeg = %d\n", numSeg);
      for (i=1; i<numSeg; ++i ) {
         k = 3*i;

         /*first set segment colors to those of previous segment*/
         colSeg[k] = colSeg[k-3];
         colSeg[k+1] = colSeg[k-2];
         colSeg[k+2] = colSeg[k-1];

         /*then add (numStdIncr) color increments*/
         for (j=0; j<numStdIncr; ++j) {
            colSeg[k] = colSeg[k] + colIncr[3*i_incr];
            colSeg[k+1] = colSeg[k+1] + colIncr[3*i_incr+1];
            colSeg[k+2] = colSeg[k+2] + colIncr[3*i_incr+2];
            ++i_incr;
         }
      }
   }

   else {
      /*color segments already indicated - no incremtation etc necessary*/
      
      colSeg = SUMA_calloc( 3*numSeg, sizeof(float));
  
      for (i=0; i<numSeg; ++i) {
         /*create color*/
         k = 3*i;
         colSeg[k]   = ((10-distTOint[i])/10)*color[colRngInt[2*i]][0] + (distTOint[i]/10)*color[colRngInt[2*i+1]][0];
         colSeg[k+1] = ((10-distTOint[i])/10)*color[colRngInt[2*i]][1] + (distTOint[i]/10)*color[colRngInt[2*i+1]][1];
         colSeg[k+2] = ((10-distTOint[i])/10)*color[colRngInt[2*i]][2] + (distTOint[i]/10)*color[colRngInt[2*i+1]][2];
      }
   }

   SUMA_free(stdColIncr);
   SUMA_free(bind_currCol);
   SUMA_free(colRngInt);
   SUMA_free(distTOint);
   if (!allGvn) {
      SUMA_free(colUsed);
      SUMA_free(numColDiv);
      SUMA_free(colIncr);
   }

   return colSeg;                                                                                                                                           
}


/*!
  array = SUMA_assignColors( vals, cols, numVal, numCol, gradRng, valDiv );

  Function to assign colors to vector of values.
  \param vals (float *) vector (size numVal) of values to be assigned colors
  \param cols (float *) vector (size 3 x numCol) of RGB colors
  \param numVal (int) number of values in vals vector
  \param numCol (int) number of colors in cols vector
  \param gradRng (float *) range of values through which color gradient changes - implies colors below range all assigned to same color (same goes for above range); passing NULL assumes min/max of vals vector taken as gradient range
  \param valDiv (float *) returned containing upper limit on value divisions (pass as NULL)
  \ret valCol (float *) 3 x numVal vector of RGB colors (or 100, if numDiv==-1) colors, in 3x(numDiv+1) format (corresponding to RGB)

  Written by Brenna Argall
*/

float * SUMA_assignColors( float *vals, float *cols, int numVal, int numCol, float *gradRng, float *valDiv ) {
   
   int i, j, k;
   float *valCol=NULL;
   float min, max, segSize=0;
   static char FuncName[]={"SUMA_assignColors"};

   SUMA_ENTRY;
   
   valCol = SUMA_calloc( 3*numVal, sizeof(float));
   valDiv = SUMA_calloc( numCol, sizeof(float));
 
   /*find min/max of vals*/
   min = vals[0]; max = vals[0];
   for (i=0; i<numVal; ++i) {
      if (vals[i]<min) min = vals[i];
      else if (vals[i]>max) max = vals[i];
   }

   if (gradRng==NULL) {
      /*if no color value range given, base segment size on full range of values*/
      segSize = (max-min)/numCol;
      /*set (high end only) value cutoffs for color segments*/
      for (i=0; i<numCol; ++i) {
         valDiv[i] = min+(i+1)*segSize;
      }
   }
   else {
      /*else base segment size on color value range given (keeping in mind two colors must be saved for values out of range)*/
      segSize = (gradRng[1]-gradRng[0])/(numCol-2);
      /*set (high end only) value cutoffs for color segments*/
      valDiv[0] = gradRng[0];
      valDiv[numCol-1] = max;
      for (i=1; i<numCol-1; ++i) {
         valDiv[i] = valDiv[0] + i*segSize;
      }
   }

   for (i=0; i<numVal; ++i) {
      /*assign segment colors*/
      j = 3*i;
      for (k=0; k<numCol; ++k) {
         if ( vals[i]<=valDiv[k] ) {
            /*value falls within segment k => assigned color k*/
            valCol[j] = cols[ 3*k ];
            valCol[j+1] = cols[ 3*k+1 ];
            valCol[j+2] = cols[ 3*k+2 ];
            break;
         }
      }
   }
   fprintf(SUMA_STDERR, "numCol = %d\n", numCol);
   /**write divisions to screen (if fairly discrete)*/
   if (numCol<20) {
      fprintf(SUMA_STDERR, "COLOR RANGES:\n\t[%f, %f]\n", min, valDiv[0]);
      for (i=1; i<numCol; ++i) {
         fprintf(SUMA_STDERR, "\t(%f, %f]\n", valDiv[i-1], valDiv[i]);
      }
      fprintf(SUMA_STDERR, "\n");
   }

   SUMA_free(valDiv);
   
   SUMA_RETURN(valCol);
}

/*!
  function used to create a SUMA_1dData structure
*/
SUMA_1dData *SUMA_Create_1dData (void) 
{
   static char FuncName[]={"SUMA_Create_1dData"};
   int i=0;

   SUMA_1dData *data = NULL;
   
   SUMA_ENTRY;
   
   data = (SUMA_1dData *) SUMA_malloc (sizeof(SUMA_1dData));
   if (!data) {
      fprintf (SUMA_STDERR, "\nError %s: Failed to allocate for MI.\n", FuncName);
      SUMA_RETURN (NULL);
   }

   data->nd_list = NULL;
   data->vxl_list = NULL;
   data->ijk_list = NULL;
   data->nvox_list = NULL;
   data->valArray = NULL;

   SUMA_RETURN (data);
}

/*!
  function to free SUMA_1dData structure
*/
SUMA_Boolean SUMA_Free_1dData (SUMA_1dData *data) 
{
   static char FuncName[]={"SUMA_Free_1dData"};
   
   SUMA_ENTRY;

   if (!data) {
      SUMA_RETURN (YUP);
   }
   if (data->nd_list) SUMA_free (data->nd_list);
   if (data->vxl_list) SUMA_free (data->vxl_list);
   if (data->ijk_list) SUMA_free (data->ijk_list);
   if (data->nvox_list) SUMA_free (data->nvox_list);
   if (data->valArray) SUMA_free (data->valArray);

   SUMA_free (data);
   
   SUMA_RETURN (YUP);
}


