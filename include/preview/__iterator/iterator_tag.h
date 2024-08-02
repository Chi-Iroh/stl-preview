//
// Created by yonggyulee on 2023/12/25.
//

#ifndef PREVIEW_ITERATOR_ITERATOR_TAG_H_
#define PREVIEW_ITERATOR_ITERATOR_TAG_H_

#include <iterator>

#include "preview/config.h"

namespace preview {

using input_iterator_tag = std::input_iterator_tag;
using output_iterator_tag = std::output_iterator_tag;
using forward_iterator_tag = std::forward_iterator_tag;
using bidirectional_iterator_tag = std::bidirectional_iterator_tag;
using random_access_iterator_tag = std::random_access_iterator_tag;

#if PREVIEW_HAVE_CONTIGUOUS_ITERATOR_TAG
using contiguous_iterator_tag = std::contiguous_iterator_tag;
#else
struct contiguous_iterator_tag : public random_access_iterator_tag {};
#endif

// placeholder for ignoring typedefs
struct iterator_ignore : input_iterator_tag {};

} // namespace preview

#endif // PREVIEW_ITERATOR_ITERATOR_TAG_H_
