
#pragma once

#ifdef GT_DEBUG

#include "gt_base/types.h"
#include <maya/MMatrix.h>
#include <maya/MGlobal.h>
#include <maya/MDagPath.h>
#include "maya/MDrawTraversal.h"

namespace gt
{

static inline void debugMatrix(MMatrix const & m)
{
  MString s("\n");

  for(int i = 0; i < 4; i++)
  {
    for(int j = 0; j < 4; j++)
    {
      s += m[i][j];
      if(j < 3)
      {
        s += ", ";
      }
    }
    s += "\n";
  }
  MGlobal::displayInfo(s);
}

class DrawTraversal : public MDrawTraversal
{
  virtual bool filterNode(const MDagPath & traversalItem)
  {
    bool prune = false;
    if(traversalItem.childCount() == 0) {
      // if(!traversalItem.hasFn(MFn::kDirectionalLight)) {
      if(!traversalItem.hasFn(MFn::kSpotLight)) {
        prune = true;
      }
    }
    return prune;
  }
};

/// display light matrices
static inline void displayLightMatrices(MDagPath const & camDagPath, i32 w, i32 h)
{
  DrawTraversal trav;
  trav.enableFiltering(true);
  trav.setFrustum(camDagPath, w, h);
  trav.traverse();
  u32 numItems = trav.numberOfItems();
  for(u32 i = 0; i < numItems; i++)
  {
    MDagPath dagPath;
    trav.itemPath(i, dagPath);
    if(!dagPath.isValid()) continue;
    MGlobal::displayInfo("\n" + dagPath.partialPathName() + "...");
    debugMatrix(dagPath.inclusiveMatrix());
  }
}

}

#endif

