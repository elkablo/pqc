#include <pqc_asymmetric_key.hpp>

namespace pqc
{

asymmetric_key::asymmetric_key() :
	has_private_(false),
	has_public_(false)
{
}

asymmetric_key::~asymmetric_key()
{
}

bool asymmetric_key::has_private() const
{
	return has_private_;
}

bool asymmetric_key::has_public() const
{
	return has_public_;
}

}
