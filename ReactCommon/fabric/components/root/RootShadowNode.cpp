/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "RootShadowNode.h"

#include <react/components/view/conversions.h>
#include <react/debug/SystraceSection.h>

namespace facebook {
namespace react {

const char RootComponentName[] = "RootView";

bool RootShadowNode::layoutIfNeeded(
    std::vector<LayoutableShadowNode const *> *affectedNodes) {
  SystraceSection s("RootShadowNode::layout");

  if (getIsLayoutClean()) {
    return false;
  }

  ensureUnsealed();

  auto layoutContext = getProps()->layoutContext;
  layoutContext.affectedNodes = affectedNodes;

  layout(layoutContext);

  // This is the rare place where shadow node must layout (set `layoutMetrics`)
  // itself because there is no a parent node which usually should do it.
  if (getHasNewLayout()) {
    setLayoutMetrics(layoutMetricsFromYogaNode(yogaNode_));
    setHasNewLayout(false);
  }

  return true;
}

RootShadowNode::Unshared RootShadowNode::clone(
    LayoutConstraints const &layoutConstraints,
    LayoutContext const &layoutContext) const {
  auto props = std::make_shared<RootProps const>(
      *getProps(), layoutConstraints, layoutContext);
  auto newRootShadowNode = std::make_shared<RootShadowNode>(
      *this,
      ShadowNodeFragment{
          /* .props = */ props,
      });
  return newRootShadowNode;
}

RootShadowNode::Unshared RootShadowNode::clone(
    ShadowNodeFamily const &shadowNodeFamily,
    std::function<ShadowNode::Unshared(ShadowNode const &oldShadowNode)>
        callback) const {
  auto ancestors = shadowNodeFamily.getAncestors(*this);

  if (ancestors.size() == 0) {
    return RootShadowNode::Unshared{nullptr};
  }

  auto &parent = ancestors.back();
  auto &oldShadowNode = parent.first.get().getChildren().at(parent.second);

  auto newShadowNode = callback(*oldShadowNode);

  auto childNode = newShadowNode;

  for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it) {
    auto &parentNode = it->first.get();
    auto childIndex = it->second;

    auto children = parentNode.getChildren();
    assert(ShadowNode::sameFamily(*children.at(childIndex), *childNode));
    children[childIndex] = childNode;

    childNode = parentNode.clone({
        ShadowNodeFragment::propsPlaceholder(),
        std::make_shared<SharedShadowNodeList>(children),
    });
  }

  return std::const_pointer_cast<RootShadowNode>(
      std::static_pointer_cast<RootShadowNode const>(childNode));
}

} // namespace react
} // namespace facebook
