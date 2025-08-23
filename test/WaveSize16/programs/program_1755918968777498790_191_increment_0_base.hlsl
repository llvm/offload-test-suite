[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 13))) {
    switch ((WaveGetLaneIndex() % 2)) {
    case 0: {
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 15))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
          }
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveSum(result));
          }
        }
        break;
      }
    case 1: {
        if (((WaveGetLaneIndex() % 2) == 0)) {
          result = (result + WaveActiveSum(2));
        }
        break;
      }
    }
    if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveMin(8));
    }
  } else {
  if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 13))) {
    result = (result + WaveActiveSum(result));
  }
  }
}
