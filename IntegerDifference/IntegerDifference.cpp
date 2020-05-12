
#include <cstdint> // fixed-size integer types
#include <cstdlib> // std::max
#include <iostream>


//static const int64_t range_of_int32_t = (int64_t)INT32_MAX - (int64_t)INT32_MIN;
//
//static const int64_t range_of_uint32_t = (int64_t)UINT32_MAX;
//
//static_assert(
//	range_of_int32_t <= range_of_uint32_t,
//	"Unexpected numerical limits. uint32_t has inadequate range."
//);

using namespace std;




// NO NO NO NO NO. BAD WRONG DANGEROUS TERRIBLE. Overflows cause undefined behaviour.
uint32_t INCORRECT_difference_int32(int32_t i, int32_t j) {
	return std::abs(i - j); // The subtraction is unsafe. The call to abs() is also unsafe.
}


// The easy way using int64_t. Of course, this approach can't be used
// if the input integer type is the largest available integer type.
uint32_t easy_difference_int32(int32_t i, int32_t j) {
	return (uint32_t)std::abs((int64_t)i - (int64_t)j);
}


// The low-level way. This strikes me as having a 2's complement flavour, but
// I'm unsure if it matters whether our target platform uses 2's complement,
// due to the way signed-to-unsigned conversions are defined in C and C++:
// >  the value is converted by repeatedly adding or subtracting one more than the maximum value
// >  that can be represented in the new type until the value is in the range of the new type
uint32_t lowlevel_difference_int32(int32_t i, int32_t j) {
	static_assert(
		(-(int64_t)INT32_MIN) == (int64_t)INT32_MAX + 1,
		"Unexpected numerical limits. This code assumes two's complement."
	);

	// Map the signed values across to the number-line of uint32_t.
	// Preserves the greater-than relation, such that an input of INT32_MIN
	// is mapped to 0, and an input of 0 is mapped to near the middle
	// of the uint32_t number-line.
	// Leverages the wrap-around behaviour of unsigned integer types.

	// It would be more intuitive to set the offset to (uint32_t)(-1 * INT32_MIN)
	// but that multiplication overflows the signed integer type,
	// causing undefined behaviour. We get the right effect subtracting from zero.
	const uint32_t offset = (uint32_t)0 - (uint32_t)(INT32_MIN);
	const uint32_t i_u = (uint32_t)i + offset;
	const uint32_t j_u = (uint32_t)j + offset;

#if 1
	// Readable version:
	const uint32_t ret = (i_u > j_u) ? (i_u - j_u) : (j_u - i_u);
#else
	// Unreadable branch-free version stolen from
	// https://graphics.stanford.edu/~seander/bithacks.html#IntegerMinOrMax
	// Modern code-generators seem to generate *worse* code from this variation!

	// Surprisingly it helps code-gen in MSVC 2019 to manually factor-out
	// the common subexpression. (Even with optimisation /O2)
	const uint32_t t = (i_u ^ j_u) & -(i_u < j_u);
	const uint32_t min = j_u ^ t; // min(i_u, j_u)
	const uint32_t max = i_u ^ t; // max(i_u, j_u)
	const uint32_t ret = max - min;
#endif
	return ret;
}


// The laborious way
uint32_t laborious_difference_int32(int32_t i, int32_t j)
{   // This static assert should pass even on 1's complement.
	// It's just about impossible that int32_t could ever be capable of representing
	// *more* values than can uint32_t.
	// Recall that in 2's complement it's the same number, but in 1's complement,
	// uint32_t can represent one more value than can int32_t.
	static_assert( // Must use int64_t to subtract negative number from INT32_MAX
		((int64_t)INT32_MAX - (int64_t)INT32_MIN) <= (int64_t)UINT32_MAX,
		"Unexpected numerical limits. Unable to represent greatest possible difference."
	);

	uint32_t ret;
	if (i == j) {
		ret = 0;
	} else {

		if (j > i) { // Swap them so that i > j
			const int32_t i_orig = i;
			i = j;
			j = i_orig;
		} // We may now safely assume i > j

		uint32_t greater_magn; // The magnitude, i.e. abs()
		bool     greater_is_negative; // Zero is of course non-negative
		uint32_t lesser_magn;
		bool     lesser_is_negative;

		if (i >= 0) {
			greater_magn = i;
			greater_is_negative = false;
		} else { // Here it follows that 'lesser' is also negative, but we'll keep the flow simple
			// greater_magn = -i; // DANGEROUS, overflows if i == INT32_MIN.
			greater_magn = (uint32_t)0 - (uint32_t)i;
			greater_is_negative = true;
		}

		if (j >= 0) {
			lesser_magn = j;
			lesser_is_negative = false;
		} else {
			// lesser_magn = -j; // DANGEROUS, overflows if i == INT32_MIN.
			lesser_magn = (uint32_t)0 - (uint32_t)j;
			lesser_is_negative = true;
		}

		// Finally compute the difference between lesser and greater
		if (!greater_is_negative && !lesser_is_negative) {
			ret = greater_magn - lesser_magn;
		} else if (greater_is_negative && lesser_is_negative) {
			ret = lesser_magn - greater_magn;
		} else { // One negative, one non-negative. Difference between them is sum of the magnitudes.
			// This looks like it may overflow, but it never will.
			ret = lesser_magn + greater_magn;
		}
	}
	return ret;
}



uint32_t difference_int32(int32_t i, int32_t j) {
	const auto v1 = laborious_difference_int32(i, j);
	const auto v2 = easy_difference_int32(i, j);
	const auto v3 = lowlevel_difference_int32(i, j);

	const bool ok = (v1 == v2) && (v2 == v3);
	if (!ok) {
		cout << "CHECK FAILED. The 3 implementations did not agree." << endl;
	}
	//else {
	//	cout << "Check passed." << endl;
	//}
	return v1;
}



int main()
{
	cout << "Starting..." << endl;

	const auto d1 = difference_int32(0, 0);
	const bool check1 = (d1 == 0);
	cout << "Check 1: " << check1 << endl;

	const auto d2 = difference_int32(0, INT32_MAX);
	const bool check2 = (d2 == INT32_MAX);
	cout << "Check 2: " << check2 << endl;

	// This one causes overflow in naive implementation
	const auto d3 = difference_int32(INT32_MIN, INT32_MAX);
	const bool check3_1 = ((int64_t)d3 == (int64_t)INT32_MAX - (int64_t)INT32_MIN);
	const bool check3_2 = (d3 == UINT32_MAX);
	cout << "Check 3_1: " << check3_1 << endl;
	cout << "Check 3_2: " << check3_2 << endl;

	const auto d4 = difference_int32(-14, 20);
	const bool check4 = (d4 == 34);
	cout << "Check 4: " << check4 << endl;


	const auto d5 = difference_int32(INT32_MIN, INT32_MIN + 1);
	const bool check5 = (d5 == 1);
	cout << "Check 5: " << check5 << endl;

	const auto d6 = difference_int32(-400, -300);
	const bool check6 = (d6 == 100);
	cout << "Check 6: " << check6 << endl;

	const auto d7 = difference_int32(-300, -400);
	const bool check7 = (d7 == 100);
	cout << "Check 7: " << check7 << endl;

	const auto d8 = difference_int32(400, 300);
	const bool check8 = (d8 == 100);
	cout << "Check 8: " << check8 << endl;

	const auto d9 = difference_int32(300, 400);
	const bool check9 = (d9 == 100);
	cout << "Check 9: " << check9 << endl;

	const auto d10 = difference_int32(20, -14); // 4 but commuted
	const bool check10 = (d10 == 34);
	cout << "Check 10: " << check10 << endl;

	const auto d11 = difference_int32(INT32_MIN + 1, INT32_MIN); // 5 but commuted
	const bool check11 = (d11 == 1);
	cout << "Check 11: " << check11 << endl;

	const auto d12 = difference_int32(21, 1021);
	const bool check12 = (d12 == 1000);
	cout << "Check 12: " << check12 << endl;

	const auto d13 = difference_int32(1021, 21);
	const bool check13 = (d13 == 1000);
	cout << "Check 13: " << check13 << endl;

	// This one causes overflow in naive implementation
	const auto d14 = difference_int32(INT32_MAX, INT32_MIN); // 3 but commuted
	const bool check14_1 = ((int64_t)d14 == (int64_t)INT32_MAX - (int64_t)INT32_MIN);
	const bool check14_2 = (d14 == UINT32_MAX);
	cout << "Check 14_1: " << check14_1 << endl;
	cout << "Check 14_2: " << check14_2 << endl;

	const auto d15 = difference_int32(INT32_MAX, INT32_MAX);
	const bool check15 = (d15 == 0);
	cout << "Check 15: " << check15 << endl;

	const auto d16 = difference_int32(INT32_MIN, INT32_MIN);
	const bool check16 = (d16 == 0);
	cout << "Check 16: " << check16 << endl;

	const auto d17 = difference_int32(INT32_MAX, 0); // 2 but commuted
	const bool check17 = (d17 == INT32_MAX);
	cout << "Check 17: " << check17 << endl;

	const auto d18 = difference_int32(12345, 12345);
	const bool check18 = (d18 == 0);
	cout << "Check 18: " << check18 << endl;

	const auto d19 = difference_int32(-45678, -45678);
	const bool check19 = (d19 == 0);
	cout << "Check 19: " << check19 << endl;

	const auto d20 = difference_int32(INT32_MAX - 234, INT32_MAX);
	const bool check20 = (d20 == 234);
	cout << "Check 20: " << check20 << endl;

	const auto d21 = difference_int32(INT32_MAX, INT32_MAX - 234);
	const bool check21 = (d21 == 234);
	cout << "Check 21: " << check21 << endl;

	const auto d22 = difference_int32(INT32_MIN + 234, INT32_MIN);
	const bool check22 = (d22 == 234);
	cout << "Check 22: " << check22 << endl;

	const auto d23 = difference_int32(INT32_MIN, INT32_MIN + 234);
	const bool check23 = (d23 == 234);
	cout << "Check 23: " << check23 << endl;

	// This one causes overflow in naive implementation
	const int64_t abs_int32_min_as_int64 = -1 * (int64_t)(INT32_MIN);
	const auto d24 = difference_int32(INT32_MIN, 0);
	const bool check24 = ((int64_t)d24 == abs_int32_min_as_int64);
	cout << "Check 24: " << check24 << endl;

	// This one causes overflow in naive implementation
	const auto d25 = difference_int32(0, INT32_MIN);
	const bool check25 = ((int64_t)d25 == abs_int32_min_as_int64);
	cout << "Check 25: " << check25 << endl;


	return 0;
}
