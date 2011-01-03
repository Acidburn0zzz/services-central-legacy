/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Corporation code.
 *
 * The Initial Developer of the Original Code is Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Bas Schouten <bschouten@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "ContainerLayerD3D9.h"
#include "gfxUtils.h"
#include "nsRect.h"

namespace mozilla {
namespace layers {

ContainerLayerD3D9::ContainerLayerD3D9(LayerManagerD3D9 *aManager)
  : ContainerLayer(aManager, NULL)
  , LayerD3D9(aManager)
{
  mImplData = static_cast<LayerD3D9*>(this);
}

ContainerLayerD3D9::~ContainerLayerD3D9()
{
  while (mFirstChild) {
    RemoveChild(mFirstChild);
  }
}

void
ContainerLayerD3D9::InsertAfter(Layer* aChild, Layer* aAfter)
{
  aChild->SetParent(this);
  if (!aAfter) {
    Layer *oldFirstChild = GetFirstChild();
    mFirstChild = aChild;
    aChild->SetNextSibling(oldFirstChild);
    aChild->SetPrevSibling(nsnull);
    if (oldFirstChild) {
      oldFirstChild->SetPrevSibling(aChild);
    } else {
      mLastChild = aChild;
    }
    NS_ADDREF(aChild);
    return;
  }
  for (Layer *child = GetFirstChild();
       child; child = child->GetNextSibling()) {
    if (aAfter == child) {
      Layer *oldNextSibling = child->GetNextSibling();
      child->SetNextSibling(aChild);
      aChild->SetNextSibling(oldNextSibling);
      if (oldNextSibling) {
        oldNextSibling->SetPrevSibling(aChild);
      } else {
        mLastChild = aChild;
      }
      aChild->SetPrevSibling(child);
      NS_ADDREF(aChild);
      return;
    }
  }
  NS_WARNING("Failed to find aAfter layer!");
}

void
ContainerLayerD3D9::RemoveChild(Layer *aChild)
{
  if (GetFirstChild() == aChild) {
    mFirstChild = GetFirstChild()->GetNextSibling();
    if (mFirstChild) {
      mFirstChild->SetPrevSibling(nsnull);
    } else {
      mLastChild = nsnull;
    }
    aChild->SetNextSibling(nsnull);
    aChild->SetPrevSibling(nsnull);
    aChild->SetParent(nsnull);
    NS_RELEASE(aChild);
    return;
  }
  Layer *lastChild = nsnull;
  for (Layer *child = GetFirstChild(); child;
       child = child->GetNextSibling()) {
    if (child == aChild) {
      // We're sure this is not our first child. So lastChild != NULL.
      lastChild->SetNextSibling(child->GetNextSibling());
      if (child->GetNextSibling()) {
        child->GetNextSibling()->SetPrevSibling(lastChild);
      } else {
        mLastChild = lastChild;
      }
      child->SetNextSibling(nsnull);
      child->SetPrevSibling(nsnull);
      child->SetParent(nsnull);
      NS_RELEASE(aChild);
      return;
    }
    lastChild = child;
  }
}

Layer*
ContainerLayerD3D9::GetLayer()
{
  return this;
}

LayerD3D9*
ContainerLayerD3D9::GetFirstChildD3D9()
{
  if (!mFirstChild) {
    return nsnull;
  }
  return static_cast<LayerD3D9*>(mFirstChild->ImplData());
}

static inline LayerD3D9*
GetNextSiblingD3D9(LayerD3D9* aLayer)
{
   Layer* layer = aLayer->GetLayer()->GetNextSibling();
   return layer ? static_cast<LayerD3D9*>(layer->
                                          ImplData())
                 : nsnull;
}

static PRBool
HasOpaqueAncestorLayer(Layer* aLayer)
{
  for (Layer* l = aLayer->GetParent(); l; l = l->GetParent()) {
    if (l->GetContentFlags() & Layer::CONTENT_OPAQUE)
      return PR_TRUE;
  }
  return PR_FALSE;
}

void
ContainerLayerD3D9::RenderLayer()
{
  nsRefPtr<IDirect3DSurface9> previousRenderTarget;
  nsRefPtr<IDirect3DTexture9> renderTexture;
  float previousRenderTargetOffset[4];
  RECT oldClipRect;
  float renderTargetOffset[] = { 0, 0, 0, 0 };
  float oldViewMatrix[4][4];

  nsIntRect visibleRect = mVisibleRegion.GetBounds();
  PRBool useIntermediate = UseIntermediateSurface();

  mSupportsComponentAlphaChildren = PR_FALSE;
  gfxMatrix contTransform;
  if (useIntermediate) {
    device()->GetRenderTarget(0, getter_AddRefs(previousRenderTarget));
    device()->CreateTexture(visibleRect.width, visibleRect.height, 1,
                            D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
                            D3DPOOL_DEFAULT, getter_AddRefs(renderTexture),
                            NULL);
    nsRefPtr<IDirect3DSurface9> renderSurface;
    renderTexture->GetSurfaceLevel(0, getter_AddRefs(renderSurface));
    device()->SetRenderTarget(0, renderSurface);

    if (mVisibleRegion.GetNumRects() == 1 && (GetContentFlags() & CONTENT_OPAQUE)) {
      // don't need a background, we're going to paint all opaque stuff
      mSupportsComponentAlphaChildren = PR_TRUE;
    } else {
      const gfx3DMatrix& transform3D = GetEffectiveTransform();
      gfxMatrix transform;
      // If we have an opaque ancestor layer, then we can be sure that
      // all the pixels we draw into are either opaque already or will be
      // covered by something opaque. Otherwise copying up the background is
      // not safe.
      HRESULT hr = E_FAIL;
      if (HasOpaqueAncestorLayer(this) &&
          transform3D.Is2D(&transform) && !transform.HasNonIntegerTranslation()) {
        // Copy background up from below
        RECT dest = { 0, 0, visibleRect.width, visibleRect.height };
        RECT src = dest;
        ::OffsetRect(&src,
                     visibleRect.x + PRInt32(transform.x0),
                     visibleRect.y + PRInt32(transform.y0));
        hr = device()->
          StretchRect(previousRenderTarget, &src, renderSurface, &dest, D3DTEXF_NONE);
      }
      if (hr == S_OK) {
        mSupportsComponentAlphaChildren = PR_TRUE;
      } else {
        device()->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 0), 0, 0);
      }
    }

    device()->GetVertexShaderConstantF(CBvRenderTargetOffset, previousRenderTargetOffset, 1);
    renderTargetOffset[0] = (float)visibleRect.x;
    renderTargetOffset[1] = (float)visibleRect.y;
    device()->SetVertexShaderConstantF(CBvRenderTargetOffset, renderTargetOffset, 1);

    gfx3DMatrix viewMatrix;
    /*
     * Matrix to transform to viewport space ( <-1.0, 1.0> topleft,
     * <1.0, -1.0> bottomright)
     */
    viewMatrix._11 = 2.0f / visibleRect.width;
    viewMatrix._22 = -2.0f / visibleRect.height;
    viewMatrix._41 = -1.0f;
    viewMatrix._42 = 1.0f;

    device()->GetVertexShaderConstantF(CBmProjection, &oldViewMatrix[0][0], 4);
    device()->SetVertexShaderConstantF(CBmProjection, &viewMatrix._11, 4);
  } else {
#ifdef DEBUG
    PRBool is2d =
#endif
    GetEffectiveTransform().Is2D(&contTransform);
    NS_ASSERTION(is2d, "Transform must be 2D");
  }

  /*
   * Render this container's contents.
   */
  for (LayerD3D9* layerToRender = GetFirstChildD3D9();
       layerToRender != nsnull;
       layerToRender = GetNextSiblingD3D9(layerToRender)) {

    const nsIntRect* clipRect = layerToRender->GetLayer()->GetClipRect();
    if ((clipRect && clipRect->IsEmpty()) ||
        layerToRender->GetLayer()->GetEffectiveVisibleRegion().IsEmpty()) {
      continue;
    }

    if (clipRect || useIntermediate) {
      RECT r;
      device()->GetScissorRect(&oldClipRect);
      if (clipRect) {
        r.left = (LONG)(clipRect->x - renderTargetOffset[0]);
        r.top = (LONG)(clipRect->y - renderTargetOffset[1]);
        r.right = (LONG)(clipRect->x - renderTargetOffset[0] + clipRect->width);
        r.bottom = (LONG)(clipRect->y - renderTargetOffset[1] + clipRect->height);
      } else {
        r.left = 0;
        r.top = 0;
        r.right = visibleRect.width;
        r.bottom = visibleRect.height;
      }

      nsRefPtr<IDirect3DSurface9> renderSurface;
      device()->GetRenderTarget(0, getter_AddRefs(renderSurface));

      D3DSURFACE_DESC desc;
      renderSurface->GetDesc(&desc);

      if (!useIntermediate) {
        // Transform clip rect
        if (clipRect) {
          gfxRect cliprect(r.left, r.top, r.right - r.left, r.bottom - r.top);
          gfxRect trScissor = contTransform.TransformBounds(cliprect);
          trScissor.Round();
          nsIntRect trIntScissor;
          if (gfxUtils::GfxRectToIntRect(trScissor, &trIntScissor)) {
            r.left = trIntScissor.x;
            r.top = trIntScissor.y;
            r.right = trIntScissor.XMost();
            r.bottom = trIntScissor.YMost();
          } else {
            r.left = 0;
            r.top = 0;
            r.right = visibleRect.width;
            r.bottom = visibleRect.height;
            clipRect = nsnull;
          }
        }
        // Intersect with current clip rect.
        r.left = NS_MAX<PRInt32>(oldClipRect.left, r.left);
        r.right = NS_MIN<PRInt32>(oldClipRect.right, r.right);
        r.top = NS_MAX<PRInt32>(oldClipRect.top, r.top);
        r.bottom = NS_MIN<PRInt32>(oldClipRect.bottom, r.bottom);
      } else {
        // > 0 is implied during the intersection when useIntermediate == true;
        r.left = NS_MAX<LONG>(0, r.left);
        r.top = NS_MAX<LONG>(0, r.top);
      }
      r.bottom = NS_MIN<LONG>(r.bottom, desc.Height);
      r.right = NS_MIN<LONG>(r.right, desc.Width);

      device()->SetScissorRect(&r);
    }

    layerToRender->RenderLayer();

    if (clipRect || useIntermediate) {
      device()->SetScissorRect(&oldClipRect);
    }
  }

  if (useIntermediate) {
    device()->SetRenderTarget(0, previousRenderTarget);
    device()->SetVertexShaderConstantF(CBvRenderTargetOffset, previousRenderTargetOffset, 1);
    device()->SetVertexShaderConstantF(CBmProjection, &oldViewMatrix[0][0], 4);

    device()->SetVertexShaderConstantF(CBvLayerQuad,
                                       ShaderConstantRect(visibleRect.x,
                                                          visibleRect.y,
                                                          visibleRect.width,
                                                          visibleRect.height),
                                       1);

    SetShaderTransformAndOpacity();

    mD3DManager->SetShaderMode(DeviceManagerD3D9::RGBALAYER);

    device()->SetTexture(0, renderTexture);
    device()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
  }
}

void
ContainerLayerD3D9::LayerManagerDestroyed()
{
  while (mFirstChild) {
    GetFirstChildD3D9()->LayerManagerDestroyed();
    RemoveChild(mFirstChild);
  }
}

} /* layers */
} /* mozilla */
