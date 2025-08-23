RWStructuredBuffer<uint> _participant_check_sum : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = 0;
  uint result = 0;
  if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
    uint counter0 = 0;
    while ((counter0 < 3)) {
      counter0 = (counter0 + 1);
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
          result = (result + WaveActiveMax(7));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 5);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
    }
  }
  if (((WaveGetLaneIndex() & 1) == 0)) {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 8);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
    uint counter2 = 0;
    while ((counter2 < 3)) {
      counter2 = (counter2 + 1);
      if ((WaveGetLaneIndex() == 11)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 0);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      if ((WaveGetLaneIndex() < 4)) {
        if ((WaveGetLaneIndex() == 10)) {
          if ((WaveGetLaneIndex() == 9)) {
            result = (result + WaveActiveSum(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if ((WaveGetLaneIndex() == 11)) {
            result = (result + WaveActiveSum(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
      if ((WaveGetLaneIndex() == 12)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 1);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMin(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 1);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 3);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMin(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 3);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if ((i4 == 1)) {
            continue;
          }
        }
        if ((WaveGetLaneIndex() < 3)) {
          result = (result + WaveActiveMax(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 2);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 15))) {
        if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 9))) {
          result = (result + WaveActiveSum(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
          if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9))) {
            result = (result + WaveActiveMax(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveSum(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
      break;
    }
  default: {
      result = (result + WaveActiveSum(99));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 0);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      break;
    }
  }
}
