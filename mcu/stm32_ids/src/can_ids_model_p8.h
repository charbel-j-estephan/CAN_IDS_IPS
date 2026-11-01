


    // !!! This file is generated using emlearn !!!

    #include <stdint.h>

// Where the model's code goes. By default these are ordinary functions. A
// firmware can define CAN_IDS_FN / CAN_IDS_PUBLIC before including this file
// to put them in a linker section, for example RAM (stm32_ids/src/classify.c).
#ifndef CAN_IDS_FN
#define CAN_IDS_FN static inline
#endif
#ifndef CAN_IDS_PUBLIC
#define CAN_IDS_PUBLIC
#endif

    

CAN_IDS_FN int32_t can_ids_tree_0(const float *features, int32_t features_length) {
          if (features[9] < 254.500000f) {
              if (features[0] < 0.500000f) {
                  if (features[7] < 0.500000f) {
                      if (features[5] < 99.500000f) {
                          return 1;
                      } else {
                          return 2;
                      }
                  } else {
                      return 2;
                  }
              } else {
                  if (features[6] < 66.500000f) {
                      if (features[3] < 191.500000f) {
                          if (features[0] < 1520.500000f) {
                              if (features[9] < 20.500000f) {
                                  if (features[6] < 38.500000f) {
                                      if (features[4] < 128.500000f) {
                                          if (features[6] < 18.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[6] < 15.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[7] < 133.500000f) {
                                          if (features[0] < 688.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[8] < 7.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[7] < 150.500000f) {
                                      if (features[0] < 1201.500000f) {
                                          if (features[5] < 48.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[4] < 0.500000f) {
                                          if (features[3] < 1.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[4] < 35.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[1] < 6.500000f) {
                                  return 0;
                              } else {
                                  if (features[6] < 0.500000f) {
                                      if (features[4] < 3.500000f) {
                                          return 0;
                                      } else {
                                          if (features[2] < 4.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      } else {
                          if (features[1] < 6.500000f) {
                              return 0;
                          } else {
                              if (features[8] < 0.500000f) {
                                  if (features[6] < 19.500000f) {
                                      if (features[9] < 241.500000f) {
                                          if (features[0] < 1530.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[7] < 19.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[3] < 238.500000f) {
                                      if (features[9] < 20.500000f) {
                                          if (features[0] < 499.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 497.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 1263.500000f) {
                                          return 2;
                                      } else {
                                          if (features[2] < 0.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[3] < 64.500000f) {
                          if (features[0] < 607.500000f) {
                              if (features[2] < 0.500000f) {
                                  if (features[5] < 254.500000f) {
                                      if (features[5] < 1.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 0;
                                  }
                              } else {
                                  if (features[8] < 0.500000f) {
                                      if (features[5] < 3.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[0] < 1088.500000f) {
                                  if (features[4] < 244.500000f) {
                                      if (features[6] < 104.500000f) {
                                          if (features[9] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 36.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[8] < 0.500000f) {
                                      if (features[2] < 4.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[4] < 6.500000f) {
                                          if (features[3] < 0.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[7] < 0.500000f) {
                              if (features[5] < 254.500000f) {
                                  if (features[9] < 0.500000f) {
                                      if (features[8] < 19.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[9] < 2.500000f) {
                                      if (features[2] < 1.500000f) {
                                          if (features[6] < 106.500000f) {
                                              return 0;
                                          } else {
                                              return 3;
                                          }
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      if (features[2] < 20.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              if (features[3] < 130.500000f) {
                                  if (features[2] < 32.500000f) {
                                      if (features[0] < 1087.500000f) {
                                          if (features[8] < 27.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[3] < 126.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[8] < 21.500000f) {
                                          if (features[0] < 1087.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 790.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[5] < 254.500000f) {
                                      if (features[3] < 156.500000f) {
                                          if (features[6] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[2] < 71.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[4] < 1.500000f) {
                                          if (features[0] < 1174.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      }
                  }
              }
          } else {
              if (features[4] < 35.500000f) {
                  if (features[8] < 47.500000f) {
                      if (features[4] < 0.500000f) {
                          if (features[3] < 120.500000f) {
                              return 0;
                          } else {
                              if (features[7] < 68.500000f) {
                                  return 2;
                              } else {
                                  return 0;
                              }
                          }
                      } else {
                          if (features[5] < 254.500000f) {
                              if (features[6] < 6.500000f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          } else {
                              return 0;
                          }
                      }
                  } else {
                      if (features[8] < 253.500000f) {
                          if (features[2] < 0.500000f) {
                              if (features[7] < 76.500000f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          } else {
                              return 2;
                          }
                      } else {
                          if (features[3] < 133.500000f) {
                              return 0;
                          } else {
                              return 2;
                          }
                      }
                  }
              } else {
                  if (features[2] < 68.500000f) {
                      if (features[3] < 33.500000f) {
                          if (features[8] < 11.500000f) {
                              return 0;
                          } else {
                              return 2;
                          }
                      } else {
                          if (features[7] < 1.500000f) {
                              return 0;
                          } else {
                              if (features[6] < 254.500000f) {
                                  return 2;
                              } else {
                                  return 0;
                              }
                          }
                      }
                  } else {
                      if (features[4] < 36.500000f) {
                          if (features[5] < 242.500000f) {
                              if (features[5] < 164.500000f) {
                                  if (features[3] < 118.500000f) {
                                      return 2;
                                  } else {
                                      return 0;
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              return 4;
                          }
                      } else {
                          if (features[7] < 0.500000f) {
                              if (features[6] < 82.500000f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          } else {
                              return 2;
                          }
                      }
                  }
              }
          }
        }
        

CAN_IDS_FN int32_t can_ids_tree_1(const float *features, int32_t features_length) {
          if (features[9] < 254.500000f) {
              if (features[0] < 0.500000f) {
                  if (features[7] < 0.500000f) {
                      return 1;
                  } else {
                      return 2;
                  }
              } else {
                  if (features[6] < 66.500000f) {
                      if (features[4] < 128.500000f) {
                          if (features[4] < 16.500000f) {
                              if (features[0] < 1520.500000f) {
                                  if (features[2] < 27.500000f) {
                                      if (features[3] < 136.500000f) {
                                          if (features[3] < 128.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[9] < 19.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[7] < 147.500000f) {
                                          if (features[8] < 47.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[8] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[7] < 0.500000f) {
                                      if (features[5] < 19.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[9] < 154.500000f) {
                                  if (features[5] < 48.500000f) {
                                      if (features[3] < 192.500000f) {
                                          if (features[7] < 152.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 1206.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[7] < 0.500000f) {
                                          if (features[2] < 114.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[6] < 7.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[5] < 37.500000f) {
                                      if (features[5] < 36.500000f) {
                                          if (features[6] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 1157.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 193.500000f) {
                                          if (features[2] < 183.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[6] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[9] < 125.500000f) {
                              if (features[8] < 33.500000f) {
                                  if (features[2] < 117.500000f) {
                                      if (features[8] < 2.500000f) {
                                          if (features[3] < 232.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 64.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 1.500000f) {
                                          if (features[8] < 3.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[9] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[2] < 5.500000f) {
                                      if (features[0] < 799.500000f) {
                                          if (features[2] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[6] < 0.500000f) {
                                          if (features[7] < 29.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[5] < 22.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[0] < 1200.500000f) {
                                  return 2;
                              } else {
                                  if (features[5] < 3.500000f) {
                                      if (features[7] < 1.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[0] < 1201.500000f) {
                                          if (features[8] < 27.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[8] < 0.500000f) {
                          if (features[0] < 848.500000f) {
                              if (features[4] < 244.500000f) {
                                  if (features[7] < 138.500000f) {
                                      if (features[7] < 0.500000f) {
                                          return 0;
                                      } else {
                                          if (features[0] < 310.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[4] < 251.500000f) {
                                      return 2;
                                  } else {
                                      if (features[5] < 67.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              if (features[3] < 14.500000f) {
                                  if (features[0] < 1061.500000f) {
                                      return 2;
                                  } else {
                                      if (features[5] < 10.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[7] < 0.500000f) {
                                      if (features[3] < 69.500000f) {
                                          if (features[2] < 5.500000f) {
                                              return 3;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[2] < 27.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 254.500000f) {
                                          return 2;
                                      } else {
                                          if (features[2] < 17.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[4] < 154.500000f) {
                              if (features[3] < 130.500000f) {
                                  if (features[2] < 36.500000f) {
                                      if (features[0] < 1087.500000f) {
                                          if (features[0] < 303.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 17.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 5.500000f) {
                                          if (features[9] < 128.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 305.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[8] < 17.500000f) {
                                      if (features[3] < 145.500000f) {
                                          if (features[7] < 137.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[7] < 20.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 790.500000f) {
                                          if (features[0] < 789.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              if (features[8] < 34.500000f) {
                                  if (features[5] < 31.500000f) {
                                      if (features[7] < 32.500000f) {
                                          if (features[7] < 17.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[7] < 41.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 790.500000f) {
                                          if (features[5] < 32.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[2] < 80.500000f) {
                                      if (features[2] < 70.500000f) {
                                          if (features[5] < 34.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[8] < 121.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[4] < 155.500000f) {
                                          if (features[9] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      }
                  }
              }
          } else {
              if (features[2] < 68.500000f) {
                  if (features[3] < 130.500000f) {
                      if (features[4] < 198.500000f) {
                          if (features[8] < 48.500000f) {
                              if (features[7] < 136.500000f) {
                                  if (features[5] < 116.500000f) {
                                      if (features[2] < 53.500000f) {
                                          if (features[3] < 99.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[7] < 91.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[8] < 16.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[7] < 253.500000f) {
                                  return 2;
                              } else {
                                  return 0;
                              }
                          }
                      } else {
                          if (features[0] < 1982.500000f) {
                              if (features[7] < 5.500000f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[8] < 235.500000f) {
                                  return 2;
                              } else {
                                  return 0;
                              }
                          }
                      }
                  } else {
                      if (features[8] < 2.500000f) {
                          return 0;
                      } else {
                          return 2;
                      }
                  }
              } else {
                  if (features[2] < 69.500000f) {
                      if (features[5] < 238.500000f) {
                          if (features[0] < 418.500000f) {
                              return 0;
                          } else {
                              return 2;
                          }
                      } else {
                          return 4;
                      }
                  } else {
                      if (features[5] < 0.500000f) {
                          if (features[4] < 19.500000f) {
                              if (features[8] < 112.500000f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          } else {
                              return 2;
                          }
                      } else {
                          if (features[5] < 254.500000f) {
                              if (features[8] < 0.500000f) {
                                  if (features[5] < 10.500000f) {
                                      return 2;
                                  } else {
                                      if (features[7] < 119.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[2] < 172.500000f) {
                                  if (features[7] < 152.500000f) {
                                      if (features[8] < 107.500000f) {
                                          if (features[7] < 77.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 0;
                              }
                          }
                      }
                  }
              }
          }
        }
        

CAN_IDS_FN int32_t can_ids_tree_2(const float *features, int32_t features_length) {
          if (features[5] < 143.500000f) {
              if (features[0] < 0.500000f) {
                  if (features[4] < 3.500000f) {
                      return 1;
                  } else {
                      return 2;
                  }
              } else {
                  if (features[3] < 191.500000f) {
                      if (features[6] < 36.500000f) {
                          if (features[7] < 150.500000f) {
                              if (features[9] < 21.500000f) {
                                  if (features[0] < 1520.500000f) {
                                      if (features[5] < 20.500000f) {
                                          if (features[6] < 18.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[0] < 1352.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[8] < 3.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[8] < 136.500000f) {
                                      if (features[3] < 128.500000f) {
                                          if (features[2] < 41.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[7] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[9] < 25.500000f) {
                                          if (features[6] < 22.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 48.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[0] < 498.500000f) {
                                  if (features[6] < 15.500000f) {
                                      if (features[9] < 16.500000f) {
                                          if (features[0] < 398.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[4] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 496.500000f) {
                                          return 2;
                                      } else {
                                          if (features[5] < 25.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[7] < 152.500000f) {
                                      if (features[0] < 623.500000f) {
                                          if (features[4] < 56.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[2] < 16.500000f) {
                                          if (features[3] < 28.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[8] < 189.500000f) {
                              if (features[5] < 105.500000f) {
                                  if (features[7] < 2.500000f) {
                                      if (features[3] < 140.500000f) {
                                          if (features[0] < 1681.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[5] < 48.500000f) {
                                          if (features[3] < 128.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[2] < 254.500000f) {
                                      if (features[6] < 254.500000f) {
                                          if (features[2] < 3.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[3] < 4.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 246.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  }
                              }
                          } else {
                              if (features[2] < 17.500000f) {
                                  if (features[6] < 254.500000f) {
                                      return 2;
                                  } else {
                                      if (features[4] < 128.500000f) {
                                          if (features[0] < 563.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[8] < 250.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[6] < 254.500000f) {
                                      return 2;
                                  } else {
                                      if (features[8] < 199.500000f) {
                                          if (features[7] < 160.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[3] < 59.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[5] < 7.500000f) {
                          if (features[7] < 0.500000f) {
                              if (features[0] < 1251.500000f) {
                                  if (features[4] < 251.500000f) {
                                      if (features[9] < 29.500000f) {
                                          return 0;
                                      } else {
                                          if (features[5] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[2] < 0.500000f) {
                                  if (features[6] < 32.500000f) {
                                      if (features[8] < 230.500000f) {
                                          return 0;
                                      } else {
                                          if (features[8] < 238.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[9] < 20.500000f) {
                              if (features[7] < 32.500000f) {
                                  if (features[5] < 140.500000f) {
                                      if (features[8] < 2.500000f) {
                                          if (features[0] < 449.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[9] < 1.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 0;
                                  }
                              } else {
                                  if (features[3] < 238.500000f) {
                                      if (features[6] < 20.500000f) {
                                          if (features[8] < 128.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[8] < 0.500000f) {
                                  if (features[0] < 1195.500000f) {
                                      return 2;
                                  } else {
                                      if (features[3] < 254.500000f) {
                                          if (features[6] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[9] < 24.500000f) {
                                      if (features[3] < 192.500000f) {
                                          if (features[2] < 4.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 499.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  }
              }
          } else {
              if (features[9] < 254.500000f) {
                  if (features[5] < 254.500000f) {
                      if (features[6] < 9.500000f) {
                          if (features[6] < 0.500000f) {
                              if (features[8] < 0.500000f) {
                                  if (features[0] < 418.500000f) {
                                      return 2;
                                  } else {
                                      return 0;
                                  }
                              } else {
                                  if (features[9] < 0.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[2] < 0.500000f) {
                                  if (features[9] < 12.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[6] < 18.500000f) {
                              if (features[4] < 168.500000f) {
                                  if (features[9] < 18.500000f) {
                                      if (features[9] < 15.500000f) {
                                          if (features[5] < 144.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 23.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[6] < 15.500000f) {
                                      if (features[7] < 168.500000f) {
                                          if (features[4] < 169.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[6] < 24.500000f) {
                                  if (features[2] < 0.500000f) {
                                      if (features[7] < 54.500000f) {
                                          if (features[0] < 532.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[9] < 31.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[3] < 0.500000f) {
                                      if (features[5] < 170.500000f) {
                                          if (features[0] < 2007.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[7] < 0.500000f) {
                          if (features[2] < 1.500000f) {
                              if (features[6] < 106.500000f) {
                                  return 0;
                              } else {
                                  if (features[6] < 110.500000f) {
                                      return 3;
                                  } else {
                                      return 0;
                                  }
                              }
                          } else {
                              if (features[9] < 8.500000f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[8] < 34.500000f) {
                              if (features[6] < 127.500000f) {
                                  if (features[7] < 109.500000f) {
                                      if (features[6] < 85.500000f) {
                                          return 2;
                                      } else {
                                          if (features[9] < 27.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 150.500000f) {
                                          if (features[4] < 102.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[7] < 138.500000f) {
                                      if (features[3] < 153.500000f) {
                                          if (features[4] < 13.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              return 2;
                          }
                      }
                  }
              } else {
                  if (features[3] < 41.500000f) {
                      if (features[7] < 44.500000f) {
                          if (features[5] < 254.500000f) {
                              if (features[7] < 2.500000f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[0] < 640.500000f) {
                                  return 2;
                              } else {
                                  return 4;
                              }
                          }
                      } else {
                          if (features[8] < 252.500000f) {
                              return 2;
                          } else {
                              return 0;
                          }
                      }
                  } else {
                      if (features[8] < 15.500000f) {
                          if (features[3] < 149.500000f) {
                              if (features[4] < 28.500000f) {
                                  if (features[5] < 235.500000f) {
                                      return 2;
                                  } else {
                                      return 0;
                                  }
                              } else {
                                  if (features[6] < 28.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[7] < 8.500000f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          return 2;
                      }
                  }
              }
          }
        }
        

CAN_IDS_FN int32_t can_ids_tree_3(const float *features, int32_t features_length) {
          if (features[0] < 0.500000f) {
              if (features[6] < 1.500000f) {
                  return 1;
              } else {
                  return 2;
              }
          } else {
              if (features[9] < 254.500000f) {
                  if (features[5] < 142.500000f) {
                      if (features[4] < 154.500000f) {
                          if (features[0] < 1680.500000f) {
                              if (features[8] < 221.500000f) {
                                  if (features[7] < 148.500000f) {
                                      if (features[8] < 47.500000f) {
                                          if (features[4] < 1.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[0] < 496.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 48.500000f) {
                                          if (features[0] < 607.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[2] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[2] < 0.500000f) {
                                      if (features[8] < 241.500000f) {
                                          if (features[7] < 147.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[7] < 179.500000f) {
                                          return 2;
                                      } else {
                                          if (features[7] < 180.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[1] < 6.500000f) {
                                  return 0;
                              } else {
                                  if (features[4] < 2.500000f) {
                                      if (features[6] < 0.500000f) {
                                          if (features[3] < 111.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[7] < 0.500000f) {
                                          if (features[2] < 5.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[6] < 169.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[6] < 122.500000f) {
                              if (features[0] < 848.500000f) {
                                  if (features[0] < 789.500000f) {
                                      if (features[9] < 15.500000f) {
                                          if (features[6] < 34.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[9] < 16.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[8] < 49.500000f) {
                                          if (features[7] < 57.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[5] < 3.500000f) {
                                      if (features[2] < 113.500000f) {
                                          if (features[3] < 135.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 1205.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 0.500000f) {
                                          if (features[0] < 1206.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              if (features[7] < 21.500000f) {
                                  if (features[2] < 53.500000f) {
                                      if (features[2] < 52.500000f) {
                                          if (features[7] < 0.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[4] < 155.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[1] < 7.500000f) {
                                          return 0;
                                      } else {
                                          if (features[3] < 0.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[8] < 123.500000f) {
                                      if (features[5] < 48.500000f) {
                                          if (features[6] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[7] < 0.500000f) {
                          if (features[2] < 1.500000f) {
                              if (features[3] < 66.500000f) {
                                  return 0;
                              } else {
                                  if (features[9] < 3.500000f) {
                                      if (features[6] < 106.500000f) {
                                          return 0;
                                      } else {
                                          return 3;
                                      }
                                  } else {
                                      if (features[4] < 213.500000f) {
                                          if (features[4] < 7.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[2] < 0.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[8] < 26.500000f) {
                                  if (features[0] < 1051.500000f) {
                                      return 2;
                                  } else {
                                      if (features[3] < 224.500000f) {
                                          if (features[0] < 1402.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[9] < 249.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[2] < 33.500000f) {
                              if (features[5] < 254.500000f) {
                                  if (features[6] < 18.500000f) {
                                      if (features[0] < 497.500000f) {
                                          if (features[3] < 238.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[0] < 497.500000f) {
                                          if (features[9] < 24.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[7] < 123.500000f) {
                                      if (features[8] < 39.500000f) {
                                          if (features[6] < 3.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[8] < 34.500000f) {
                                          if (features[3] < 152.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              if (features[4] < 0.500000f) {
                                  if (features[0] < 304.500000f) {
                                      if (features[8] < 17.500000f) {
                                          if (features[0] < 237.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[9] < 0.500000f) {
                                      if (features[4] < 96.500000f) {
                                          if (features[3] < 69.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[5] < 254.500000f) {
                                          if (features[3] < 0.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[8] < 16.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      }
                  }
              } else {
                  if (features[2] < 68.500000f) {
                      if (features[4] < 197.500000f) {
                          if (features[3] < 130.500000f) {
                              if (features[7] < 138.500000f) {
                                  if (features[8] < 52.500000f) {
                                      if (features[5] < 116.500000f) {
                                          if (features[7] < 81.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 253.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[4] < 2.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[8] < 252.500000f) {
                                      return 2;
                                  } else {
                                      return 0;
                                  }
                              }
                          } else {
                              if (features[5] < 253.500000f) {
                                  if (features[8] < 2.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 0;
                              }
                          }
                      } else {
                          if (features[0] < 1982.500000f) {
                              if (features[0] < 1191.500000f) {
                                  return 2;
                              } else {
                                  if (features[6] < 1.500000f) {
                                      return 0;
                                  } else {
                                      if (features[7] < 243.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  }
                              }
                          } else {
                              if (features[0] < 2014.500000f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          }
                      }
                  } else {
                      if (features[3] < 41.500000f) {
                          if (features[2] < 69.500000f) {
                              return 4;
                          } else {
                              if (features[6] < 3.500000f) {
                                  return 0;
                              } else {
                                  if (features[3] < 0.500000f) {
                                      if (features[7] < 67.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      } else {
                          if (features[8] < 16.500000f) {
                              if (features[5] < 3.500000f) {
                                  return 0;
                              } else {
                                  if (features[5] < 254.500000f) {
                                      if (features[8] < 1.500000f) {
                                          if (features[5] < 15.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[3] < 87.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  }
                              }
                          } else {
                              return 2;
                          }
                      }
                  }
              }
          }
        }
        

CAN_IDS_FN int32_t can_ids_tree_4(const float *features, int32_t features_length) {
          if (features[9] < 254.500000f) {
              if (features[0] < 0.500000f) {
                  if (features[6] < 0.500000f) {
                      return 1;
                  } else {
                      return 2;
                  }
              } else {
                  if (features[4] < 40.500000f) {
                      if (features[4] < 0.500000f) {
                          if (features[8] < 209.500000f) {
                              if (features[0] < 1707.500000f) {
                                  if (features[8] < 47.500000f) {
                                      if (features[6] < 71.500000f) {
                                          if (features[6] < 28.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[8] < 18.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 10.500000f) {
                                          if (features[8] < 82.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[6] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[7] < 1.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[9] < 17.500000f) {
                                  if (features[0] < 553.500000f) {
                                      if (features[0] < 472.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[2] < 41.500000f) {
                              if (features[0] < 1680.500000f) {
                                  if (features[8] < 178.500000f) {
                                      if (features[8] < 138.500000f) {
                                          if (features[8] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[9] < 63.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 254.500000f) {
                                          if (features[7] < 38.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[7] < 194.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[7] < 0.500000f) {
                                      if (features[5] < 27.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[2] < 254.500000f) {
                                  if (features[8] < 2.500000f) {
                                      if (features[7] < 59.500000f) {
                                          if (features[5] < 20.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[9] < 2.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[2] < 55.500000f) {
                                          if (features[8] < 37.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[8] < 24.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[6] < 98.500000f) {
                                      if (features[8] < 9.500000f) {
                                          if (features[4] < 31.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[9] < 129.500000f) {
                                          if (features[6] < 254.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[5] < 105.500000f) {
                          if (features[7] < 39.500000f) {
                              if (features[2] < 220.500000f) {
                                  if (features[6] < 155.500000f) {
                                      if (features[8] < 221.500000f) {
                                          if (features[0] < 1442.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[6] < 3.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[7] < 0.500000f) {
                                          if (features[9] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[6] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[0] < 1202.500000f) {
                                      if (features[0] < 1200.500000f) {
                                          if (features[8] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[7] < 19.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[6] < 254.500000f) {
                                  if (features[9] < 20.500000f) {
                                      if (features[0] < 809.500000f) {
                                          if (features[9] < 19.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[5] < 48.500000f) {
                                          if (features[5] < 47.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[9] < 22.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[3] < 157.500000f) {
                                      if (features[4] < 174.500000f) {
                                          if (features[9] < 129.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      } else {
                          if (features[3] < 64.500000f) {
                              if (features[4] < 96.500000f) {
                                  if (features[3] < 63.500000f) {
                                      if (features[7] < 0.500000f) {
                                          if (features[0] < 1159.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[9] < 5.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[8] < 13.500000f) {
                                          if (features[4] < 92.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[6] < 5.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[6] < 15.500000f) {
                                      if (features[7] < 0.500000f) {
                                          if (features[5] < 243.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[6] < 5.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 24.500000f) {
                                          if (features[9] < 24.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              if (features[4] < 96.500000f) {
                                  if (features[9] < 0.500000f) {
                                      if (features[5] < 254.500000f) {
                                          if (features[8] < 7.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[8] < 0.500000f) {
                                              return 3;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 5.500000f) {
                                          if (features[9] < 7.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[2] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[8] < 0.500000f) {
                                      if (features[7] < 1.500000f) {
                                          if (features[2] < 87.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[7] < 97.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[9] < 15.500000f) {
                                          if (features[9] < 5.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[6] < 24.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      }
                  }
              }
          } else {
              if (features[5] < 254.500000f) {
                  if (features[5] < 105.500000f) {
                      if (features[8] < 47.500000f) {
                          if (features[4] < 196.500000f) {
                              if (features[3] < 132.500000f) {
                                  if (features[0] < 1224.500000f) {
                                      if (features[2] < 90.500000f) {
                                          if (features[6] < 215.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[4] < 26.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[6] < 0.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[6] < 1.500000f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[6] < 254.500000f) {
                              if (features[7] < 0.500000f) {
                                  if (features[4] < 102.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[3] < 74.500000f) {
                                  return 2;
                              } else {
                                  return 0;
                              }
                          }
                      }
                  } else {
                      if (features[8] < 0.500000f) {
                          if (features[6] < 89.500000f) {
                              return 0;
                          } else {
                              return 2;
                          }
                      } else {
                          if (features[6] < 254.500000f) {
                              return 2;
                          } else {
                              if (features[7] < 222.500000f) {
                                  return 2;
                              } else {
                                  return 0;
                              }
                          }
                      }
                  }
              } else {
                  if (features[3] < 54.500000f) {
                      if (features[2] < 58.500000f) {
                          return 0;
                      } else {
                          return 4;
                      }
                  } else {
                      if (features[8] < 55.500000f) {
                          if (features[0] < 330.500000f) {
                              return 0;
                          } else {
                              return 2;
                          }
                      } else {
                          return 2;
                      }
                  }
              }
          }
        }
        

CAN_IDS_PUBLIC int32_t can_ids_predict(const float *features, int32_t features_length) {

        int32_t votes[5] = {0,};
        int32_t _class = -1;

        _class = can_ids_tree_0(features, features_length); votes[_class] += 1;
    _class = can_ids_tree_1(features, features_length); votes[_class] += 1;
    _class = can_ids_tree_2(features, features_length); votes[_class] += 1;
    _class = can_ids_tree_3(features, features_length); votes[_class] += 1;
    _class = can_ids_tree_4(features, features_length); votes[_class] += 1;
    
        int32_t most_voted_class = -1;
        int32_t most_voted_votes = 0;
        for (int32_t i=0; i<5; i++) {

            if (votes[i] > most_voted_votes) {
                most_voted_class = i;
                most_voted_votes = votes[i];
            }
        }
        return most_voted_class;
    }
    

int can_ids_predict_proba(const float *features, int32_t features_length, float *out, int out_length) {

        int32_t _class = -1;

        for (int i=0; i<out_length; i++) {
            out[i] = 0.0f;
        }

        _class = can_ids_tree_0(features, features_length); out[_class] += 1.0f;
    _class = can_ids_tree_1(features, features_length); out[_class] += 1.0f;
    _class = can_ids_tree_2(features, features_length); out[_class] += 1.0f;
    _class = can_ids_tree_3(features, features_length); out[_class] += 1.0f;
    _class = can_ids_tree_4(features, features_length); out[_class] += 1.0f;
    
        // compute mean
        for (int i=0; i<out_length; i++) {
            out[i] = out[i] / 5;
        }
        return 0;
    }
    
// Which model this is: shown by the firmware in READY and #STAT.
#define CAN_IDS_MODEL_ID "rf5-d9-p8-c0f7e8b5"

// CAN IDs in the normal training traffic (38 of 2048), one bit per ID.
static const uint8_t can_ids_known[256] = {
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x46, 0x00, 0x03, 0x00, 0x00
};

CAN_IDS_FN int can_ids_id_known(uint32_t can_id)
{
    return (can_ids_known[(can_id >> 3) & 0xFF] >> (can_id & 7)) & 1;
}
