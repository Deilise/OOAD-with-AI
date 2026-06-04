#pragma once

#include "rvc/Ports.hpp"

namespace rvc {

class SurfaceCleaningController final : public DustInputPort {
public:
    explicit SurfaceCleaningController(CleaningCommandSink& cleaningSink);

    void SessionStateChanged(bool active);
    void DustSignalUpdated(DustSignal signal) override;
    void SuspendCleaningForManeuver();
    void ResumeCleaningAfterManeuver();

private:
    void send(CleaningCommand command);

    CleaningMode cleaningMode_{CleaningMode::Suspended};
    PowerLevel powerLevel_{PowerLevel::Off};
    DustSignal dustSignal_{DustSignal::normal};
    bool sessionActive_{false};
    CleaningCommandSink& cleaningSink_;
};

} // namespace rvc
