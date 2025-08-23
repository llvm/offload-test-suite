RWStructuredBuffer<uint> _participant_check_sum : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = 0;
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
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
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 5))) {
            if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 7))) {
              if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 5))) {
                result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
                uint _participantCount = WaveActiveSum(1);
                bool _isCorrect = (_participantCount == 0);
                _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
              }
            }
            if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 2))) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
              uint _participantCount = WaveActiveSum(1);
              bool _isCorrect = (_participantCount == 0);
              _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
            }
          }
          break;
        }
      case 2: {
          for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
            if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
              result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
              uint _participantCount = WaveActiveSum(1);
              bool _isCorrect = (_participantCount == 1);
              _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
            }
            if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
              if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
                result = (result + WaveActiveSum(result));
                uint _participantCount = WaveActiveSum(1);
                bool _isCorrect = (_participantCount == 2);
                _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
              }
              if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
                result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
                uint _participantCount = WaveActiveSum(1);
                bool _isCorrect = (_participantCount == 1);
                _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
              }
            }
          }
          break;
        }
      }
      break;
    }
  case 1: {
      if ((WaveGetLaneIndex() == 13)) {
        if ((WaveGetLaneIndex() == 2)) {
          result = (result + WaveActiveSum(result));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
        if ((WaveGetLaneIndex() == 13)) {
          if ((WaveGetLaneIndex() == 1)) {
            result = (result + WaveActiveMax(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
          switch ((WaveGetLaneIndex() % 2)) {
          case 0: {
              if ((WaveGetLaneIndex() < 8)) {
                result = (result + WaveActiveSum(1));
                uint _participantCount = WaveActiveSum(1);
                bool _isCorrect = (_participantCount == 0);
                _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
              }
              break;
            }
          case 1: {
              if (((WaveGetLaneIndex() % 2) == 0)) {
                result = (result + WaveActiveSum(2));
                uint _participantCount = WaveActiveSum(1);
                bool _isCorrect = (_participantCount == 0);
                _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
              }
              break;
            }
          }
          if ((WaveGetLaneIndex() == 3)) {
            result = (result + WaveActiveMin(result));
            uint _participantCount = WaveActiveSum(1);
            bool _isCorrect = (_participantCount == 0);
            _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
          }
        }
        if ((WaveGetLaneIndex() == 11)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
          uint _participantCount = WaveActiveSum(1);
          bool _isCorrect = (_participantCount == 0);
          _participant_check_sum[tid.x] = (_participant_check_sum[tid.x] + (_isCorrect ? 1 : 0));
        }
      }
      break;
    }
  }
}
