#include <iostream>

#include <pqc_random.hpp>
#include <pqc_sidh_key_basic.hpp>

namespace pqc
{

sidh_key_basic::sidh_key_basic(const sidh_params& params) :
	asymmetric_key(),
	has_isogeny_(false),
	params_(params),
	curve_(std::make_shared<WeierstrassCurve>(params.prime)),
	P_image_(curve_),
	Q_image_(curve_)
{
}

sidh_key_basic::~sidh_key_basic()
{
}

bool sidh_key_basic::generate_private()
{
	if (has_private_)
		return true;
	else if (has_public_)
		return false;

	if (random_u32_below(get_params().l+1)) {
		m_ = 1;
		n_ = random_z_below(get_params().le);
	} else {
		m_ = random_z_below(get_params().lem1) * get_params().l;
		n_ = 1;
	}

	has_private_ = true;
	has_public_ = false;
	has_isogeny_ = false;

	return true;
}

bool sidh_key_basic::ensure_has_isogeny()
{
	if (has_isogeny_)
		return true;

	if (!has_private_)
		return false;

	WeierstrassPoint generator = m_*get_params().P + n_*get_params().Q;
	isogeny_ = WeierstrassIsogeny(generator, get_params().l, get_params().e, get_params().strategy);

	has_isogeny_ = true;

	return true;
}

std::string sidh_key_basic::compute_shared_secret(const sidh_key_basic& public_key)
{
	if (!has_private() || !public_key.has_public())
		return std::string();

	if (get_params().s == public_key.get_params().s)
		return std::string();

	const Z& m = get_m();
	const Z& n = get_n();
	int l = get_params().l;
	int e = get_params().e;
	const WeierstrassPoint& P_image = public_key.get_P_image();
	const WeierstrassPoint& Q_image = public_key.get_Q_image();

	WeierstrassPoint generator = m*P_image + n*Q_image;

	return WeierstrassIsogeny(generator, l, e, get_params().strategy).image()->j_invariant().serialize();
}

bool sidh_key_basic::generate_public()
{
	if (has_public_)
		return true;

	if (!ensure_has_isogeny())
		return false;

	curve_ = isogeny_.image();
	P_image_ = isogeny_(get_params().P_peer);
	Q_image_ = isogeny_(get_params().Q_peer);

	has_public_ = true;

	return true;
}

void sidh_key_basic::generate()
{
	has_private_ = false;
	has_public_ = false;
	has_isogeny_ = false;

	generate_private();
	generate_public();
}

std::string sidh_key_basic::export_private() const
{
	if (!has_private_)
		return std::string();

	size_t size = get_params().le.size();
	if (m_ == 1)
		return ((char) 0) + n_.serialize(size);
	else
		return ((char) 1) + m_.serialize(size);
}

std::string sidh_key_basic::export_public() const
{
	if (!has_public_)
		return std::string();

	const WeierstrassCurvePtr& curve = has_isogeny_ ? isogeny_.image() : curve_;

	return curve->serialize() + P_image_.serialize() + Q_image_.serialize();
}

std::string sidh_key_basic::export_both() const
{
	if (!has_private_ || !has_public_)
		return std::string();

	return export_private() + export_public();
}

bool sidh_key_basic::import_private(const std::string& input)
{
	Z m, n;

	if (input.size() != get_params().le.size() + 1)
		return false;

	if (input[0] == ((char) 0)) {
		m = 1;
		n.unserialize(input.substr(1));

		if (n >= get_params().le)
			return false;
	} else if (input[0] == ((char) 1)) {
		m.unserialize(input.substr(1));
		n = 1;

		if (m >= get_params().le)
			return false;
	} else {
		return false;
	}

	m_ = m;
	n_ = n;

	has_private_ = true;
	has_public_ = false;
	has_isogeny_ = false;

	return true;
}

bool sidh_key_basic::import_public(const std::string& input)
{
	size_t curve_size = curve_->size();
	size_t point_size = P_image_.size();

	if (input.size() != curve_size + 2*point_size)
		return false;

	WeierstrassCurvePtr curve = std::make_shared<WeierstrassCurve>(get_params().prime);

	if (!curve->unserialize(input.substr(0, curve_size)))
		return false;

	WeierstrassPoint P_image(curve);
	WeierstrassPoint Q_image(curve);

	if (!P_image.unserialize(input.substr(curve_size, point_size)))
		return false;

	if (!Q_image.unserialize(input.substr(curve_size + point_size, point_size)))
		return false;

	curve_ = curve;
	P_image_ = P_image;
	Q_image_ = Q_image;

	has_private_ = false;
	has_isogeny_ = false;
	has_public_ = true;

	return true;
}

bool sidh_key_basic::import(const std::string& input)
{
	size_t private_size = get_params().le.size() + 1;
	size_t public_size = curve_->size() + 2*P_image_.size();

	if (input.size() == private_size) {
		return import_private(input);
	} else if (input.size() == public_size) {
		return import_public(input);
	} else if (input.size() == private_size + public_size) {
		Z old_m = m_, old_n = n_;
		bool old_has_private = has_private_, old_has_isogeny = has_isogeny_;

		if (!import_private(input.substr(0, private_size))) {
			return false;
		} else if (!import_public(input.substr(private_size))) {
			m_ = old_m;
			n_ = old_n;
			has_private_ = old_has_private;
			has_isogeny_ = old_has_isogeny;
			return false;
		} else {
			has_private_ = true;
			has_public_ = true;
			has_isogeny_ = false;
			return true;
		}
	} else {
		return false;
	}
}

const sidh_params& sidh_key_basic::get_params() const
{
	return params_;
}

const Z& sidh_key_basic::get_m() const
{
	return m_;
}

const Z& sidh_key_basic::get_n() const
{
	return n_;
}

const WeierstrassIsogeny& sidh_key_basic::get_isogeny()
{
	ensure_has_isogeny();
	return isogeny_;
}

const WeierstrassPoint& sidh_key_basic::get_P_image() const
{
	return P_image_;
}

const WeierstrassPoint& sidh_key_basic::get_Q_image() const
{
	return Q_image_;
}

const WeierstrassCurvePtr& sidh_key_basic::get_curve_image() const
{
	return curve_;
}

}
