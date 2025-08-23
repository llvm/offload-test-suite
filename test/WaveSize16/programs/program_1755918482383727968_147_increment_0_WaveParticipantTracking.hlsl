RWStructuredBuffer<uint> _participant_check_sum : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = 0;
  uint result = 0;
  if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 1))) {
    if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 7))) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 1);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
    if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 12))) {
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
        result = (result + WaveActiveMin(10));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 2);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 14))) {
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 2);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 2);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        if ((i0 == 1)) {
          continue;
        }
      }
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 2);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
    }
    if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 1))) {
      result = (result + WaveActiveMin(8));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 1);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
  } else {
  if ((WaveGetLaneIndex() < 8)) {
    result = (result + WaveActiveMax(WaveGetLaneIndex()));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 7);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  }
  uint counter1 = 0;
  while ((counter1 < 3)) {
    counter1 = (counter1 + 1);
    for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
      if ((WaveGetLaneIndex() == 9)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 1);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 2);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
      if ((WaveGetLaneIndex() == 8)) {
        result = (result + WaveActiveMax(result));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 1);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      if ((i2 == 1)) {
        continue;
      }
    }
    if ((WaveGetLaneIndex() >= 15)) {
      result = (result + WaveActiveSum(result));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 1);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
    if ((counter1 == 2)) {
      break;
    }
  }
  if ((WaveGetLaneIndex() < 2)) {
    result = (result + WaveActiveMin(5));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 1);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  }
  }
}
