//==================================================== file = genzipf.c =====
//=  Program to generate Zipf (power law) distributed random variables      =
//===========================================================================
//=  Author: Kenneth J. Christensen                                         =
//=          University of South Florida                                    =
//=          WWW: http://www.csee.usf.edu/~christen                         =
//=          Email: christen@csee.usf.edu                                   =
//=-------------------------------------------------------------------------=
//=  History: KJC (11/16/03) - Genesis (from genexp.c)                      =
//===========================================================================
//----- Include files -------------------------------------------------------
#include <assert.h>             // Needed for assert() macro
#include <stdio.h>              // Needed for printf()
#include <stdlib.h>             // Needed for exit() and ato*()
#include <math.h>               // Needed for pow()

//----- Constants -----------------------------------------------------------
#define  FALSE          0       // Boolean false
#define  TRUE           1       // Boolean true

namespace util {

class Zipf {
  public:
    Zipf(double alpha, int n): alpha_(alpha), n_(n), first_(true), c_(0) {}

    int Generate() {
      //===========================================================================
      //=  Function to generate Zipf (power law) distributed random variables     =
      //=    - Input: alpha and N                                                 =
      //=    - Output: Returns with Zipf distributed random variable              =
      //===========================================================================
      double z;                     // Uniform random number (0 < z < 1)
      double sum_prob;              // Sum of probabilities
      double zipf_value;            // Computed exponential value to be returned
      int    i;                     // Loop counter

      // Compute normalization constant on first call only
      if (first_ == true)
      {
        for (i=1; i<=n_; i++)
          c_ = c_ + (1.0 / pow((double) i, alpha_));
        c_ = 1.0 / c_;
        first_ = false;
      }

      // Pull a uniform random number (0 < z < 1)
      do
      {
        z = rand_val(0);
      }
      while ((z == 0) || (z == 1));

      // Map z to the value
      sum_prob = 0;
      for (i=1; i<=n_; i++)
      {
        sum_prob = sum_prob + c_ / pow((double) i, alpha_);
        if (sum_prob >= z)
        {
          zipf_value = i;
          break;
        }
      }

      // Assert that zipf_value is between 1 and N
      assert((zipf_value >=1) && (zipf_value <= n_));

      return(zipf_value);
    }

    void SetSeed(int seed) {
      rand_val(seed);
    }

    //=========================================================================
    //= Multiplicative LCG for generating uniform(0.0, 1.0) random numbers    =
    //=   - x_n = 7^5*x_(n-1)mod(2^31 - 1)                                    =
    //=   - With x seeded to 1 the 10000th x value should be 1043618065       =
    //=   - From R. Jain, "The Art of Computer Systems Performance Analysis," =
    //=     John Wiley & Sons, 1991. (Page 443, Figure 26.2)                  =
    //=========================================================================
    double rand_val(int seed) {
      const long  a =      16807;  // Multiplier
      const long  m = 2147483647;  // Modulus
      const long  q =     127773;  // m div a
      const long  r =       2836;  // m mod a
      long        x_div_q;         // x divided by q
      long        x_mod_q;         // x modulo q
      long        x_new;           // New x value

      // Set the seed if argument is non-zero and then return zero
      if (seed > 0)
      {
        x_ = seed;
        return(0.0);
      }

      // RNG using integer arithmetic
      x_div_q = x_ / q;
      x_mod_q = x_ % q;
      x_new = (a * x_mod_q) - (r * x_div_q);
      if (x_new > 0)
        x_ = x_new;
      else
        x_ = x_new + m;

      // Return a random value between 0.0 and 1.0
      return((double) x_ / m);
    }

  private:
    double alpha_;
    int n_;
    long x_;     // Random int value
    bool first_; // Static first time flag
    double c_;   // Normalization constant
};

}  // namespace util
