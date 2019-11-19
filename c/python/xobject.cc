#include "python/xobject.h"

namespace py {

XTypeMaker::ConstructorTag XTypeMaker::constructor_tag;
XTypeMaker::DestructorTag XTypeMaker::destructor_tag;
XTypeMaker::GetSetTag XTypeMaker::getset_tag;
XTypeMaker::MethodTag XTypeMaker::method_tag;
XTypeMaker::Method0Tag XTypeMaker::method0_tag;
XTypeMaker::ReprTag XTypeMaker::repr_tag;
XTypeMaker::StrTag XTypeMaker::str_tag;
XTypeMaker::GetitemTag XTypeMaker::getitem_tag;
XTypeMaker::SetitemTag XTypeMaker::setitem_tag;
XTypeMaker::BuffersTag XTypeMaker::buffers_tag;
XTypeMaker::IterTag XTypeMaker::iter_tag;
XTypeMaker::NextTag XTypeMaker::next_tag;

} // namespace py