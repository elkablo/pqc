#ifndef PQC_SIDH_KEY_BASIC_HPP
#define PQC_SIDH_KEY_BASIC_HPP

#include <string>
#include <pqc_asymmetric_key.hpp>
#include <pqc_sidh_params.hpp>
#include <pqc_weierstrass.hpp>

namespace pqc
{

class sidh_key_basic : public asymmetric_key
{
public:
	sidh_key_basic(const sidh_params&);
	virtual ~sidh_key_basic();

	virtual std::string export_private() const;
	virtual std::string export_public() const;
	virtual std::string export_both() const;

	virtual bool import_private(const std::string&);
	virtual bool import_public(const std::string&);
	virtual bool import(const std::string&);

	virtual bool generate_private();
	virtual bool generate_public();
	virtual void generate();

	std::string compute_shared_secret(const sidh_key_basic&);

	const sidh_params& get_params() const;

	// private part
	const Z& get_m() const;
	const Z& get_n() const;
	const WeierstrassIsogeny& get_isogeny();

	// public part
	const WeierstrassPoint& get_P_image() const;
	const WeierstrassPoint& get_Q_image() const;
	const WeierstrassCurvePtr& get_curve_image() const;
private:
	bool ensure_has_isogeny();

	bool has_isogeny_;
	const sidh_params params_;
	Z m_, n_;
	WeierstrassIsogeny isogeny_;
	WeierstrassCurvePtr curve_;
	WeierstrassPoint P_image_, Q_image_;
};

}

#endif /* PQC_SIDH_KEY_BASIC_HPP */
