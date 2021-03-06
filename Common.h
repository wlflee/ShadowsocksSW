//
//  Common.h
//  ShadowsocksSW
//
//  Created by Xuhui on 16/10/11.
//  Copyright © 2016年 Xuhui. All rights reserved.
//

#ifndef Common_h
#define Common_h

#define kVersion @VERSION
#define kPacketTunnelProviderErrorDomain @"kPacketTunnelProviderErrorDomain"

typedef enum {
    kPacketTunnelProviderErrorSocks2ssServiceStartFailed,
} PacketTunnelProviderError;

#define kWormholeSelectedConfigChangedNotification @"kWormholeSelectedConfigChangedNotification"
#define kWormholeNeedShowFreeShadwosocksConfigUpdateTipNotification @"kWormholeNeedShowFreeShadwosocksConfigUpdateTipNotification"

#endif /* Common_h */
