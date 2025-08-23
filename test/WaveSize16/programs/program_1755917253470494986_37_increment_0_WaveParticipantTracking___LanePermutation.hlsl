RWStructuredBuffer<uint> _participant_check_sum : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = 0;
  uint result = 0;
  if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
    uint _perm_val_0 = ((WaveGetLaneIndex() == 13) ? 2 : ((WaveGetLaneIndex() == 8) ? 13 : 8));
    result = (result + WaveActiveSum(WaveReadLaneAt(1, _perm_val_0)));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 3);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  } else {
  if ((((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 9))) {
    result = (result + WaveActiveMin(2));
    uint _participantCount = WaveActiveSum(1);
    bool _isCorrect = (_participantCount == 3);
    _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
  }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 2);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      break;
    }
  case 1: {
      for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
        uint counter1 = 0;
        while ((counter1 < 2)) {
          counter1 = (counter1 + 1);
          if ((WaveGetLaneIndex() == 0)) {
            result = (result + WaveActiveMax(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
        if ((WaveGetLaneIndex() == 3)) {
          result = (result + WaveActiveSum(2));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
    }
  case 2: {
      if ((WaveGetLaneIndex() == 8)) {
        if ((WaveGetLaneIndex() == 12)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
          if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 7))) {
            result = (result + WaveActiveMax(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMin(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
        if ((WaveGetLaneIndex() == 2)) {
          result = (result + WaveActiveMax(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
    }
  case 3: {
      if ((WaveGetLaneIndex() < 20)) {
        result = (result + WaveActiveSum(4));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 12);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      break;
    }
  }
  if ((WaveGetLaneIndex() == 11)) {
    uint counter3 = 0;
    while ((counter3 < 3)) {
      counter3 = (counter3 + 1);
      uint counter4 = 0;
      while ((counter4 < 2)) {
        counter4 = (counter4 + 1);
        if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveMax(3));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
          if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11))) {
            result = (result + WaveActiveMax(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 1);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        } else {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveSum(9));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        if ((WaveGetLaneIndex() < 2)) {
          result = (result + WaveActiveSum(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
      if ((counter4 == 1)) {
        break;
      }
    }
    if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 14))) {
      result = (result + WaveActiveMax(result));
      uint _participantCount = WaveActiveSum(1);
      bool _isCorrect = (_participantCount == 0);
      _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
    }
  }
  }
}
