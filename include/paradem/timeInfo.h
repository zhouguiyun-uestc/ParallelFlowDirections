
#ifndef PARADEM_TIMEINFO_H
#define PARADEM_TIMEINFO_H

#include <cereal/access.hpp>

#include <cmath>
#include <algorithm>

class TimeInfo {
private:
    friend class cereal::access;
    template < class Archive > void serialize( Archive& ar ) {
        ar( calc, overall, io, vmpeak, vmhwm );
    }

public:
    double calc, overall, io;
    long   vmpeak, vmhwm;
    TimeInfo();
    TimeInfo( double calc, double overall, double io, long vmpeak, long vmhwm );
    TimeInfo& operator+=( const TimeInfo& o ) {
        calc += o.calc;
        overall += o.overall;
        io += o.io;
        vmpeak = std::max( vmpeak, o.vmpeak );
        vmhwm  = std::max( vmhwm, o.vmhwm );
        return *this;
    }
};

#endif
