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
      if ((WaveGetLaneIndex() >= 8)) {
        if ((WaveGetLaneIndex() >= 13)) {
          result = (result + WaveActiveSum(10));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 2);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        uint counter0 = 0;
        while ((counter0 < 2)) {
          counter0 = (counter0 + 1);
          if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 9))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 1);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 2))) {
            result = (result + WaveActiveMin(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 1);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
        if ((WaveGetLaneIndex() < 1)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      } else {
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 12))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
        uint _participantCount = WaveActiveSum(1);
        bool _isCorrect = (_participantCount == 2);
        _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
      }
      uint counter1 = 0;
      while ((counter1 < 3)) {
        counter1 = (counter1 + 1);
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
    }
    break;
  }
  }
}
