#include "potentials.h"
#include "multipole.h"

void Faunus::Potential::RepulsionR3::from_json(const Faunus::json &j) {
    f = j.value("prefactor", 1.0);
    e = j.value("lj-prefactor", 1.0);
    s = j.value("sigma", 1.0);
}

Faunus::Potential::RepulsionR3::RepulsionR3(const std::string &name) {
    PairPotentialBase::name = name;
}

void Faunus::Potential::RepulsionR3::to_json(Faunus::json &j) const {
    j = {{"prefactor",f}, {"lj-prefactor", e},{"sigma",s}};
}

void Faunus::Potential::CosAttract::to_json(Faunus::json &j) const {
    j = {{"eps",eps / 1.0_kJmol}, {"rc",rc / 1.0_angstrom }, {"wc", wc / 1.0_angstrom }};
}

void Faunus::Potential::CosAttract::from_json(const Faunus::json &j) {
    eps = j.at("eps").get<double>() * 1.0_kJmol;
    rc = j.at("rc").get<double>() * 1.0_angstrom ;
    wc = j.at("wc").get<double>() * 1.0_angstrom ;
    rc2 = rc * rc;
    c = pc::pi / 2 / wc;
    rcwc2 = pow((rc + wc), 2);
}

Faunus::Potential::CosAttract::CosAttract(const std::string &name) { PairPotentialBase::name=name; }

void Faunus::Potential::CoulombGalore::sfYukawa(const Faunus::json &j) {
    kappa = 1.0 / j.at("debyelength").get<double>();
    I = kappa*kappa / ( 8.0*lB*pc::pi*pc::Nav/1e27 );
    table = sf.generate( [&](double q) { return std::exp(-q*rc*kappa) - std::exp(-kappa*rc); }, 0, 1 ); // q=r/Rc
    // we could also fill in some info std::string or JSON output...
}

void Faunus::Potential::CoulombGalore::sfReactionField(const Faunus::json &j) {
    epsrf = j.at("eps_rf");
    table = sf.generate( [&](double q) { return 1 + (( epsrf - epsr ) / ( 2 * epsrf + epsr ))*q*q*q
            - 3 * ( epsrf / ( 2 * epsrf + epsr ))*q ; }, 0, 1);
    calcDielectric = [&](double M2V) {
        if(epsrf > 1e10)
            return 1 + 3*M2V;
        if(fabs(epsrf-epsr) < 1e-6)
            return 2.25*M2V + 0.25 + 0.75*sqrt(9*M2V*M2V + 2*M2V + 1);
        if(fabs(epsrf-1.0) < 1e-6)
            return ( 2*M2V + 1 ) / ( 1 - M2V );
        return 0.5 * ( 2*epsrf - 1 + sqrt( -72*M2V*M2V*epsrf
                    + 4*epsrf*epsrf + 4*epsrf + 1) ) / ( 3*M2V-1 ); // Needs to be checked!
        //return (6*M2V*epsrf + 2*epsrf + 1.0)/(1.0 + 2*epsrf - 3*M2V); // Is OK when epsr=1.0
    };
    selfenergy_prefactor = 1.5*epsrf/(2.0*epsrf + epsr); // Correct?!, see Eq.14 in DOI: 10.1021/jp510612w
    // we could also fill in some info std::string or JSON output...
}

void Faunus::Potential::CoulombGalore::sfQpotential(const Faunus::json &j) {
    order = j.value("order",300);
    table = sf.generate( [&](double q) { return qPochhammerSymbol( q, 1, order ); }, 0, 1 );
    calcDielectric = [&](double M2V) { return 1 + 3*M2V; };
    selfenergy_prefactor = 0.5;
}

void Faunus::Potential::CoulombGalore::sfYonezawa(const Faunus::json &j) {
    alpha = j.at("alpha");
    table = sf.generate( [&](double q) { return 1 - std::erfc(alpha*rc)*q + q*q; }, 0, 1 );
    calcDielectric = [&](double M2V) { return 1 + 3*M2V; };
    selfenergy_prefactor = erf(alpha*rc);
}

void Faunus::Potential::CoulombGalore::sfFanourgakis(const Faunus::json &j) {
    table = sf.generate( [&](double q) { return 1 - 1.75*q + 5.25*pow(q,5) - 7*pow(q,6) + 2.5*pow(q,7); }, 0, 1 );
    calcDielectric = [&](double M2V) { return 1 + 3*M2V; };
    selfenergy_prefactor = 0.875;
}

void Faunus::Potential::CoulombGalore::sfFennel(const Faunus::json &j) {
    alpha = j.at("alpha");
    table = sf.generate( [&](double q) { return (erfc(alpha*rc*q) - std::erfc(alpha*rc)*q + (q-1.0)*q*(std::erfc(alpha*rc)
                    + 2 * alpha * rc / std::sqrt(pc::pi) * std::exp(-alpha*alpha*rc*rc))); }, 0, 1 );
    calcDielectric = [&](double M2V) { double T = erf(alpha*rc) - (2 / (3 * sqrt(pc::pi)))
        * exp(-alpha*alpha*rc*rc) * (alpha*alpha*rc*rc * alpha*alpha*rc*rc + 2.0 * alpha*alpha*rc*rc + 3.0);
        return (((T + 2.0) * M2V + 1.0)/ ((T - 1.0) * M2V + 1.0)); };
    selfenergy_prefactor = ( erfc(alpha*rc)/2.0 + alpha*rc/sqrt(pc::pi) );
}

void Faunus::Potential::CoulombGalore::sfEwald(const Faunus::json &j) {
    alpha = j.at("alpha");
    table = sf.generate( [&](double q) { return std::erfc(alpha*rc*q); }, 0, 1 );
    calcDielectric = [&](double M2V) {
        double T = std::erf(alpha*rc) - (2 / (3 * sqrt(pc::pi)))
            * std::exp(-alpha*alpha*rc*rc) * ( 2*alpha*alpha*rc*rc + 3);
        return ((T + 2.0) * M2V + 1)/ ((T - 1) * M2V + 1);
    };
    selfenergy_prefactor = ( erfc(alpha*rc) + alpha*rc/sqrt(pc::pi)*(1.0 + std::exp(-alpha*alpha*rc2)) );
}

void Faunus::Potential::CoulombGalore::sfWolf(const Faunus::json &j) {
    alpha = j.at("alpha");
    table = sf.generate( [&](double q) { return (erfc(alpha*rc*q) - erfc(alpha*rc)*q); }, 0, 1 );
    calcDielectric = [&](double M2V) { double T = erf(alpha*rc) - (2 / (3 * sqrt(pc::pi))) * exp(-alpha*alpha*rc*rc)
        * ( 2.0 * alpha*alpha*rc*rc + 3.0);
        return (((T + 2.0) * M2V + 1.0)/ ((T - 1.0) * M2V + 1.0));};
    selfenergy_prefactor = ( erfc(alpha*rc) + alpha*rc/sqrt(pc::pi)*(1.0 + exp(-alpha*alpha*rc2)) );
}

void Faunus::Potential::CoulombGalore::sfPlain(const Faunus::json &j, double val) {
    table = sf.generate( [&](double q) { return val; }, 0, 1 );
    calcDielectric = [&](double M2V) { return (2.0*M2V + 1.0)/(1.0 - M2V); };
    selfenergy_prefactor = 0.0;
}

Faunus::Potential::CoulombGalore::CoulombGalore(const std::string &name) { PairPotentialBase::name=name; }

void Faunus::Potential::CoulombGalore::from_json(const Faunus::json &j) {
    try {
        type = j.at("type");
        rc = j.at("cutoff");
        rc2 = rc*rc;
        rc1i = 1/rc;
        epsr = j.at("epsr");
        lB = pc::lB( epsr );
        depsdt = j.value("depsdt", -0.368*pc::temperature/epsr);
        sf.setTolerance(
                j.value("utol",1e-5),j.value("ftol",1e-2) );

        if (type=="reactionfield") sfReactionField(j);
        if (type=="fanourgakis") sfFanourgakis(j);
        if (type=="qpotential") sfQpotential(j);
        if (type=="yonezawa") sfYonezawa(j);
        if (type=="yukawa") sfYukawa(j);
        if (type=="fennel") sfFennel(j);
        if (type=="plain") sfPlain(j,1);
        if (type=="ewald") sfEwald(j);
        if (type=="none") sfPlain(j,0);
        if (type=="wolf") sfWolf(j);
        if ( table.empty() )
            throw std::runtime_error(name + ": unknown coulomb type '" + type + "'" );
    }

    catch ( std::exception &e ) {
        std::cerr << "CoulombGalore error: " << e.what();
        throw;
    }
}

double Faunus::Potential::CoulombGalore::dielectric_constant(double M2V) {
    return calcDielectric( M2V );
}

void Faunus::Potential::CoulombGalore::to_json(Faunus::json &j) const {
    using namespace u8;
    j["epsr"] = epsr;
    j["T"+partial+epsilon_m + "/" + partial + "T"] = depsdt;
    j["lB"] = lB;
    j["cutoff"] = rc;
    j["type"] = type;
    if (type=="yukawa") {
        j["debyelength"] = 1.0/kappa;
        j["ionic strength"] = I;
    }
    if (type=="qpotential")
        j["order"] = order;
    if (type=="yonezawa" || type=="fennel" || type=="wolf" || type=="ewald")
        j["alpha"] = alpha;
    if (type=="reactionfield") {
        if(epsrf > 1e10)
            j[epsilon_m+"_rf"] = 2e10;
        else
            j[epsilon_m+"_rf"] = epsrf;
    }
    _roundjson(j, 5);
}

Faunus::Potential::Coulomb::Coulomb(const std::string &name) { PairPotentialBase::name=name; }

void Faunus::Potential::Coulomb::to_json(Faunus::json &j) const { j["epsr"] = pc::lB(lB); }

void Faunus::Potential::Coulomb::from_json(const Faunus::json &j) { lB = pc::lB( j.at("epsr") ); }

Faunus::Potential::FENE::FENE(const std::string &name) { PairPotentialBase::name=name; }

void Faunus::Potential::FENE::from_json(const Faunus::json &j) {
    k  = j.at("stiffness");
    r02 = std::pow( double(j.at("maxsep")), 2);
    r02inv = 1/r02;
}

void Faunus::Potential::FENE::to_json(Faunus::json &j) const {
    j = {{"stiffness",k}, {"maxsep",std::sqrt(r02)}};
}

void Faunus::Potential::to_json(Faunus::json &j, const Faunus::Potential::PairPotentialBase &base) {
    base.name.empty() ? base.to_json(j) : base.to_json(j[base.name]);
}

void Faunus::Potential::from_json(const Faunus::json &j, Faunus::Potential::PairPotentialBase &base) {
    if (!base.name.empty())
        if (j.count(base.name)==1) {
            base.from_json(j.at(base.name));
            return;
        }
    base.from_json(j);
}

void Faunus::Potential::to_json(Faunus::json &j, const std::shared_ptr<Faunus::Potential::BondData> &b) {
    json val;
    b->to_json(val);
    val["index"] = b->index;
    j = {{ b->name(), val }};
}

void Faunus::Potential::from_json(const Faunus::json &j, std::shared_ptr<Faunus::Potential::BondData> &b) {
    if (j.is_object())
        if (j.size()==1) {
            auto& key = j.begin().key();
            auto& val = j.begin().value();
            if ( key==HarmonicBond().name() )  b = std::make_shared<HarmonicBond>();
            else if ( key==FENEBond().name() ) b = std::make_shared<FENEBond>();
            else if ( key==HarmonicTorsion().name() ) b = std::make_shared<HarmonicTorsion>();
            else if ( key==GromosTorsion().name() ) b = std::make_shared<GromosTorsion>();
            else if ( key==PeriodicDihedral().name() ) b = std::make_shared<PeriodicDihedral>();
            else
                throw std::runtime_error("unknown bond type: " + key);
            b->from_json( val );
            b->index = val.at("index").get<decltype(b->index)>();
            if (b->index.size() != b->numindex())
                throw std::runtime_error("exactly " + std::to_string(b->numindex()) + " indices required for " + b->name());
            return;
        }
    throw std::runtime_error("invalid bond data");
}

void Faunus::Potential::BondData::shift(int offset) {
    for ( auto &i : index )
        i += offset;
}

bool Faunus::Potential::BondData::hasEnergyFunction() const { return energy!=nullptr; }

void Faunus::Potential::HarmonicBond::from_json(const Faunus::json &j) {
    k = j.at("k").get<double>() * 1.0_kJmol / std::pow(1.0_angstrom, 2) / 2; // k
    req = j.at("req").get<double>() * 1.0_angstrom; // req
}

void Faunus::Potential::HarmonicBond::to_json(Faunus::json &j) const {
    j = { {"k", 2*k/1.0_kJmol*1.0_angstrom*1.0_angstrom},
        {"req", req/1.0_angstrom} };
}

std::string Faunus::Potential::HarmonicBond::name() const { return "harmonic"; }

std::shared_ptr<Faunus::Potential::BondData> Faunus::Potential::HarmonicBond::clone() const { return std::make_shared<HarmonicBond>(*this); }

int Faunus::Potential::HarmonicBond::numindex() const { return 2; }

Faunus::Potential::BondData::Variant Faunus::Potential::HarmonicBond::type() const { return BondData::HARMONIC; }

std::shared_ptr<Faunus::Potential::BondData> Faunus::Potential::FENEBond::clone() const { return std::make_shared<FENEBond>(*this); }

int Faunus::Potential::FENEBond::numindex() const { return 2; }

Faunus::Potential::BondData::Variant Faunus::Potential::FENEBond::type() const { return BondData::FENE; }

void Faunus::Potential::FENEBond::from_json(const Faunus::json &j) {
    k[0] = j.at("k").get<double>() * 1.0_kJmol / std::pow(1.0_angstrom, 2);
    k[1] = std::pow( j.at("rmax").get<double>() * 1.0_angstrom, 2);
    k[2] = j.value("eps", 0.0) * 1.0_kJmol;
    k[3] = std::pow( j.value("sigma", 0.0)  * 1.0_angstrom, 2);
}

void Faunus::Potential::FENEBond::to_json(Faunus::json &j) const {
    j = {
        { "k", k[0] / (1.0_kJmol / std::pow(1.0_angstrom, 2)) },
        { "rmax", std::sqrt(k[1]) / 1.0_angstrom },
        { "eps", k[2] / 1.0_kJmol },
        { "sigma", std::sqrt(k[3]) / 1.0_angstrom } };
}

std::string Faunus::Potential::FENEBond::name() const { return "fene"; }

int Faunus::Potential::HarmonicTorsion::numindex() const { return 3; }

void Faunus::Potential::HarmonicTorsion::from_json(const Faunus::json &j) {
    k = j.at("k").get<double>() * 1.0_kJmol / std::pow(1.0_rad, 2);
    aeq = j.at("aeq").get<double>() * 1.0_deg;
}

void Faunus::Potential::HarmonicTorsion::to_json(Faunus::json &j) const {
    j = {
        {"k",  k / (1.0_kJmol / std::pow(1.0_rad, 2))},
        {"aeq",aeq / 1.0_deg}
    };
    _roundjson(j, 6);
}

Faunus::Potential::BondData::Variant Faunus::Potential::HarmonicTorsion::type() const { return BondData::HARMONIC_TORSION; }

std::string Faunus::Potential::HarmonicTorsion::name() const { return "harmonic_torsion"; }

std::shared_ptr<Faunus::Potential::BondData> Faunus::Potential::HarmonicTorsion::clone() const {
    return std::make_shared<HarmonicTorsion>(*this);
}

int Faunus::Potential::GromosTorsion::numindex() const { return 3; }

void Faunus::Potential::GromosTorsion::from_json(const Faunus::json &j) {
    k = j.at("k").get<double>() * 0.5 * 1.0_kJmol; // k
    aeq = cos(j.at("aeq").get<double>() * 1.0_deg); // cos(angle)
}

void Faunus::Potential::GromosTorsion::to_json(Faunus::json &j) const {
    j = {
        { "k", 2.0*k/1.0_kJmol },
        { "aeq", std::acos(aeq)/1.0_deg }
    };
}

Faunus::Potential::BondData::Variant Faunus::Potential::GromosTorsion::type() const { return BondData::G96_TORSION; }

std::string Faunus::Potential::GromosTorsion::name() const { return "gromos_torsion"; }

std::shared_ptr<Faunus::Potential::BondData> Faunus::Potential::GromosTorsion::clone() const {
    return std::make_shared<GromosTorsion>(*this);
}

int Faunus::Potential::PeriodicDihedral::numindex() const { return 4; }

std::shared_ptr<Faunus::Potential::BondData> Faunus::Potential::PeriodicDihedral::clone() const {
    return std::make_shared<PeriodicDihedral>(*this);
}

void Faunus::Potential::PeriodicDihedral::from_json(const Faunus::json &j) {
    k[0] = j.at("k").get<double>() * 1.0_kJmol; // k
    k[1] = j.at("n").get<double>(); // multiplicity/periodicity n
    k[2] = j.at("phi").get<double>() * 1.0_deg; // angle
}

void Faunus::Potential::PeriodicDihedral::to_json(Faunus::json &j) const {
    j = {
        { "k", k[0] / 1.0_kJmol },
        { "n", k[1] },
        { "phi", k[2] / 1.0_deg }
    };
}

Faunus::Potential::BondData::Variant Faunus::Potential::PeriodicDihedral::type() const { return BondData::PERIODIC_DIHEDRAL; }

std::string Faunus::Potential::PeriodicDihedral::name() const { return "periodic_dihedral"; }

Faunus::Potential::SASApotential::SASApotential(const std::string &name) {
    PairPotentialBase::name=name;
}

void Faunus::Potential::SASApotential::from_json(const Faunus::json &j) {
    shift = j.value("shift", true);
    conc = j.at("molarity").get<double>() * 1.0_molar;
    proberadius = j.value("radius", 1.4) * 1.0_angstrom;
}

void Faunus::Potential::SASApotential::to_json(Faunus::json &j) const {
    j["molarity"] = conc / 1.0_molar;
    j["radius"] = proberadius / 1.0_angstrom;
    j["shift"] = shift;
}


