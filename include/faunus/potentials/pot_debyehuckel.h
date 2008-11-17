#ifndef FAU_POT_DEBYEHUCKEL_H
#define FAU_POT_DEBYEHUCKEL_H
#include "faunus/potentials/base.h"
namespace Faunus {
  /*! \brief Debye-Huckel potential
   *  \author Mikael Lund
   *  
   *  Faunus::inputfile is scanned for "bjerrum", "debyelen", "LJeps".
   */
  class pot_debyehuckel : public pot_lj {
    private:
      double k;
    public:
      pot_debyehuckel( inputfile &in ) : pot_lj(in) {
        name+="/Debye-Huckel";
        k=in.getflt("debyelen",10);
        f=in.getflt("bjerrum",7.1);
        eps=eps/f;
      }
      //! \f$ \beta u/f = \frac{z_1z_2}{r}\exp(-\kappa r) + u_{lj}/f \f$
      //! \return Energy in kT/f (f=lB)
      inline double pairpot( const particle &p1, const particle &p2 ) {
        double r2=p1.sqdist(p2),
               r=sqrt(r2);
        return lj(p1,p2,r2) + p1.charge*p2.charge/r*exp(-k*r);
      }
      inline double sqdist(const point &p1, const point &p2) {
        return p1.sqdist(p2);
      }
      string info() {
        std::ostringstream o;
        o << pot_lj::info()
          << "#   Bjerrum length    = " << f     << endl
          << "#   Debye length      = " << 1./k  << endl;
        return o.str();
      }
  };
}
#endif