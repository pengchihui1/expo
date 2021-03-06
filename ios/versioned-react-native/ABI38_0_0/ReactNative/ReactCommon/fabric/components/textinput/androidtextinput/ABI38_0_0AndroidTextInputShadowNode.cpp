/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ABI38_0_0AndroidTextInputShadowNode.h"

#include <fbjni/fbjni.h>
#include <ABI38_0_0React/attributedstring/AttributedStringBox.h>
#include <ABI38_0_0React/attributedstring/TextAttributes.h>
#include <ABI38_0_0React/components/text/BaseTextShadowNode.h>
#include <ABI38_0_0React/core/LayoutConstraints.h>
#include <ABI38_0_0React/core/LayoutContext.h>
#include <ABI38_0_0React/core/conversions.h>
#include <ABI38_0_0React/jni/ReadableNativeMap.h>

using namespace ABI38_0_0facebook::jni;

namespace ABI38_0_0facebook {
namespace ABI38_0_0React {

extern const char AndroidTextInputComponentName[] = "AndroidTextInput";

void AndroidTextInputShadowNode::setContextContainer(
    ContextContainer *contextContainer) {
  ensureUnsealed();
  contextContainer_ = contextContainer;
}

AttributedString AndroidTextInputShadowNode::getAttributedString() const {
  // Use BaseTextShadowNode to get attributed string from children
  auto childTextAttributes = TextAttributes::defaultTextAttributes();
  childTextAttributes.apply(getProps()->textAttributes);
  auto attributedString =
      BaseTextShadowNode::getAttributedString(childTextAttributes, *this);

  // BaseTextShadowNode only gets children. We must detect and prepend text
  // value attributes manually.
  if (!getProps()->text.empty()) {
    auto textAttributes = TextAttributes::defaultTextAttributes();
    textAttributes.apply(getProps()->textAttributes);
    auto fragment = AttributedString::Fragment{};
    fragment.string = getProps()->text;
    fragment.textAttributes = textAttributes;
    // If the TextInput opacity is 0 < n < 1, the opacity of the TextInput and
    // text value's background will stack. This is a hack/workaround to prevent
    // that effect.
    fragment.textAttributes.backgroundColor = clearColor();
    fragment.parentShadowView = ShadowView(*this);
    attributedString.prependFragment(fragment);
  }

  return attributedString;
}

// For measurement purposes, we want to make sure that there's at least a
// single character in the string so that the measured height is greater
// than zero. Otherwise, empty TextInputs with no placeholder don't
// display at all.
AttributedString AndroidTextInputShadowNode::getPlaceholderAttributedString()
    const {
  // Return placeholder text, since text and children are empty.
  auto textAttributedString = AttributedString{};
  auto fragment = AttributedString::Fragment{};
  fragment.string = getProps()->placeholder;

  if (fragment.string.empty()) {
    fragment.string = " ";
  }

  auto textAttributes = TextAttributes::defaultTextAttributes();
  textAttributes.apply(getProps()->textAttributes);

  // If there's no text, it's possible that this Fragment isn't actually
  // appended to the AttributedString (see implementation of appendFragment)
  fragment.textAttributes = textAttributes;
  fragment.parentShadowView = ShadowView(*this);
  textAttributedString.appendFragment(fragment);

  return textAttributedString;
}

void AndroidTextInputShadowNode::setTextLayoutManager(
    SharedTextLayoutManager textLayoutManager) {
  ensureUnsealed();
  textLayoutManager_ = textLayoutManager;
}

void AndroidTextInputShadowNode::updateStateIfNeeded() {
  ensureUnsealed();

  auto ABI38_0_0ReactTreeAttributedString = getAttributedString();
  auto const &state = getStateData();

  assert(textLayoutManager_);
  assert(
      (!state.layoutManager || state.layoutManager == textLayoutManager_) &&
      "`StateData` refers to a different `TextLayoutManager`");

  // Tree is often out of sync with the value of the TextInput.
  // This is by design - don't change the value of the TextInput in the State,
  // and therefore in Java, unless the tree itself changes.
  if (state.ABI38_0_0ReactTreeAttributedString == ABI38_0_0ReactTreeAttributedString &&
      state.layoutManager == textLayoutManager_) {
    return;
  }

  // Store default TextAttributes in state.
  // In the case where the TextInput is completely empty (no value, no
  // defaultValue, no placeholder, no children) there are therefore no fragments
  // in the AttributedString, and when State is updated, it needs some way to
  // reconstruct a Fragment with default TextAttributes.
  auto defaultTextAttributes = TextAttributes::defaultTextAttributes();
  defaultTextAttributes.apply(getProps()->textAttributes);

  // Even if we're here and updating state, it may be only to update the layout
  // manager If that is the case, make sure we don't update text: pass in the
  // current attributedString unchanged, and pass in zero for the "event count"
  // so no changes are applied There's no way to prevent a state update from
  // flowing to Java, so we just ensure it's a noop in those cases.
  setStateData(AndroidTextInputState{
      (state.ABI38_0_0ReactTreeAttributedString == ABI38_0_0ReactTreeAttributedString
           ? 0
           : getProps()->mostRecentEventCount),
      (state.ABI38_0_0ReactTreeAttributedString == ABI38_0_0ReactTreeAttributedString
           ? state.attributedString
           : ABI38_0_0ReactTreeAttributedString),
      ABI38_0_0ReactTreeAttributedString,
      getProps()->paragraphAttributes,
      defaultTextAttributes,
      ShadowView(*this),
      textLayoutManager_});
}

#pragma mark - LayoutableShadowNode

Size AndroidTextInputShadowNode::measure(
    LayoutConstraints layoutConstraints) const {
  auto const &state = getStateData();

  AttributedString attributedString = state.attributedString;

  if (attributedString.isEmpty()) {
    attributedString = getPlaceholderAttributedString();
  }

  if (attributedString.isEmpty()) {
    return {0, 0};
  }

  return textLayoutManager_->measure(
      AttributedStringBox{attributedString},
      getProps()->paragraphAttributes,
      layoutConstraints);
}

void AndroidTextInputShadowNode::layout(LayoutContext layoutContext) {
  updateStateIfNeeded();
  ConcreteViewShadowNode::layout(layoutContext);
}

} // namespace ABI38_0_0React
} // namespace ABI38_0_0facebook
