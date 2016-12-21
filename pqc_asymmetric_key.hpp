#ifndef PQC_ASYMMETRIC_KEY_HPP
#define PQC_ASYMMETRIC_KEY_HPP

#include <string>
#include <memory>

namespace pqc
{

class asymmetric_key
{
public:
	asymmetric_key();
	virtual ~asymmetric_key();

	bool has_private() const;
	bool has_public() const;

	virtual std::string export_private() const = 0;
	virtual std::string export_public() const = 0;
	virtual std::string export_both() const = 0;

	virtual bool import_private(const std::string&) = 0;
	virtual bool import_public(const std::string&) = 0;
	virtual bool import(const std::string&) = 0;

protected:
	bool has_private_, has_public_;
};

}

#endif /* PQC_ASYMMETRIC_KEY_HPP */
