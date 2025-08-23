RWStructuredBuffer<uint> _participant_check_sum : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = 0;
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 4);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      break;
    }
  case 1: {
      if ((WaveGetLaneIndex() == 5)) {
        if ((WaveGetLaneIndex() == 7)) {
          result = (result + WaveActiveSum(3));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 5))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 1);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMax(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if ((i0 == 1)) {
            break;
          }
        }
        if ((WaveGetLaneIndex() == 12)) {
          result = (result + WaveActiveMin(6));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      } else {
      if ((WaveGetLaneIndex() < 7)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 2);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if ((WaveGetLaneIndex() >= 9)) {
          result = (result + WaveActiveMax(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 4);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
      if ((WaveGetLaneIndex() < 3)) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 1);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
    }
    break;
  }
  }
}
