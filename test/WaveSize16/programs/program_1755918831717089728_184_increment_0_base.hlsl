[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
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
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      uint counter0 = 0;
      while ((counter0 < 3)) {
        counter0 = (counter0 + 1);
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveSum(result));
        }
        for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
          }
          if ((i1 == 1)) {
            continue;
          }
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
        }
      }
      break;
    }
  case 2: {
      if ((WaveGetLaneIndex() < 1)) {
        if ((WaveGetLaneIndex() < 7)) {
          if ((WaveGetLaneIndex() < 4)) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
          }
          if ((WaveGetLaneIndex() < 4)) {
            result = (result + WaveActiveMin(5));
          }
        }
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  case 3: {
      if ((WaveGetLaneIndex() < 20)) {
        result = (result + WaveActiveSum(4));
      }
      break;
    }
  }
}
