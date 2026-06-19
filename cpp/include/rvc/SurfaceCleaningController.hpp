#pragma once

#include "rvc/Ports.hpp"

namespace rvc {

class SurfaceCleaningController final : public DustInputPort {
public:
    explicit SurfaceCleaningController(CleaningCommandSink& cleaningSink);

    void SessionStateChanged(bool active);
    void SessionStateChanged(bool active, CleaningMode cleaningMode);
    void DustSignalUpdated(DustSignal signal) override;
    void SuspendCleaningForManeuver();
    void ResumeCleaningAfterManeuver(CleaningMode mode = CleaningMode::Normal);
    void StartBoostCleaning();
    void EndBoostCleaning();
    void SetCleaningMode(CleaningMode mode);

    DustSignal dustSignal() const noexcept { return dustSignal_; }

private:
    void send(CleaningCommand command);

    CleaningMode cleaningMode_{CleaningMode::Suspended};
    PowerLevel powerLevel_{PowerLevel::Off};
    DustSignal dustSignal_{DustSignal::normal};
    bool sessionActive_{false};
    CleaningCommandSink& cleaningSink_;
};

} // namespace rvc
