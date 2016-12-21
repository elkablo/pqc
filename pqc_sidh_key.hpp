#ifndef PQC_SIDH_KEY_HPP
#define PQC_SIDH_KEY_HPP

#include <string>
#include <pqc_asymmetric_key.hpp>
#include <pqc_sidh_params.hpp>
#include <pqc_weierstrass.hpp>

namespace pqc
{

class sidh_key : public asymmetric_key
{
public:
	sidh_key(const sidh_params&);
	~sidh_key();

	std::string export_private() const;
	std::string export_public() const;
	std::string export_both() const;

	bool import_private(const std::string&);
	bool import_public(const std::string&);
	bool import(const std::string&);

	bool generate_private();
	bool generate_public();
	void generate();

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

#endif /* PQC_SIDH_KEY_HPP */
