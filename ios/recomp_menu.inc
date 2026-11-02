// UIKit methods included inside GameController. Input remains live for this menu
// while guest execution, GPU submission, rumble and audio are parked.
- (void)syncPause {
    bool pause=self.menuOpen || !self.appActive;
    if(pause){
        ios_lifecycle::gate().pause();
        if(self.started)irq_request();
        ios_lifecycle::gate().wait_for_renderer();
        if(self.started)ios_audio_suspend();
        ios_rumble_update(false);[self.touchControls resetInput];
        PadState neutral{};neutral.connected=ios_controller_snapshot().connected;pad_set_state(0,neutral);
    }else if(ios_lifecycle::gate().resume()){
        ++fpsEpoch;if(self.started)ios_audio_resume();
    }
}
- (void)foregroundChanged:(BOOL)active {
    self.appActive=active;
    self.inputLink.paused=!active;
    self.previousButtons=ios_controller_snapshot().buttons;
    [self syncPause];
}
- (void)toggleRecomp {
    if(!self.started)return;
    self.menuOpen=!self.menuOpen;self.waitingAction=0;self.menuMode=0;self.menuIndex=0;
    [self syncPause];
    if(self.menuOpen)[self showRecompPage];else{[self.recompPanel removeFromSuperview];self.recompPanel=nil;++fpsEpoch;}
}
- (void)showRecompPage {
    [self.recompPanel removeFromSuperview];
    self.recompPanel=[[UIView alloc] initWithFrame:self.view.bounds];
    self.recompPanel.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
    self.recompPanel.backgroundColor=[UIColor colorWithWhite:0 alpha:.85];
    [self.view addSubview:self.recompPanel];
    self.recompStack=[[UIStackView alloc] init];
    self.recompStack.axis=UILayoutConstraintAxisVertical;self.recompStack.spacing=4;
    self.recompStack.translatesAutoresizingMaskIntoConstraints=NO;
    self.recompTitle=[[UILabel alloc] init];self.recompTitle.textColor=UIColor.whiteColor;
    self.recompTitle.textAlignment=NSTextAlignmentCenter;self.recompTitle.numberOfLines=2;
    self.recompTitle.text=self.waitingAction?@"지정할 패드 버튼을 누르세요\nGuide 또는 아래 버튼으로 취소":self.menuMode?@"패드 버튼 지정":@"Recomp 메뉴";
    [self.recompStack addArrangedSubview:self.recompTitle];
    NSMutableArray<NSString*>* titles=[NSMutableArray array];
    if(self.waitingAction){[titles addObject:@"취소"];}
    else if(self.menuMode){
        const uint16_t actions[]={PAD_A,PAD_B,PAD_Y,PAD_X,PAD_Z,PAD_START};
        NSArray* names=@[@"가속",@"감속",@"부스터",@"사이드 어택",@"스핀 어택",@"게임 메뉴 (Start)"];
        NSArray* physical=@[@"A",@"B",@"X",@"Y",@"LB",@"RB",@"Start",@"십자키 ↑",@"십자키 ↓",@"십자키 ←",@"십자키 →"];
        auto map=ios_controller_bindings();
        for(unsigned a=0;a<6;++a){NSString* button=@"미지정";for(unsigned i=0;i<map.size();++i)if(map[i]==actions[a])button=physical[i];[titles addObject:[NSString stringWithFormat:@"%@ : %@",names[a],button]];}
        [titles addObjectsFromArray:@[@"기본 배치로 복원",@"뒤로"]];
    }else [titles addObjectsFromArray:@[[NSString stringWithFormat:@"Smooth motion : %@",self.smoothEnabled?@"켜짐":@"꺼짐"],[NSString stringWithFormat:@"FPS 표시 : %@",self.fpsEnabled?@"켜짐":@"꺼짐"],@"패드 버튼 지정",@"게임으로 돌아가기"]];
    NSMutableArray* buttons=[NSMutableArray array];
    for(unsigned i=0;i<titles.count;++i){UIButton* b=[UIButton buttonWithType:UIButtonTypeSystem];[b setTitle:titles[i] forState:UIControlStateNormal];b.tag=i;b.titleLabel.font=[UIFont systemFontOfSize:16];[b.heightAnchor constraintEqualToConstant:32].active=YES;[b addTarget:self action:@selector(menuButton:) forControlEvents:UIControlEventTouchUpInside];[self.recompStack addArrangedSubview:b];[buttons addObject:b];}
    self.menuButtons=buttons;self.menuIndex=MIN(self.menuIndex,(NSInteger)buttons.count-1);
    [self.recompPanel addSubview:self.recompStack];
    [NSLayoutConstraint activateConstraints:@[[self.recompStack.centerXAnchor constraintEqualToAnchor:self.recompPanel.centerXAnchor],[self.recompStack.centerYAnchor constraintEqualToAnchor:self.recompPanel.centerYAnchor],[self.recompStack.widthAnchor constraintEqualToAnchor:self.recompPanel.safeAreaLayoutGuide.widthAnchor multiplier:.75]]];
    [self highlightMenu];
}
- (void)highlightMenu {for(unsigned i=0;i<self.menuButtons.count;++i)self.menuButtons[i].backgroundColor=(NSInteger)i==self.menuIndex?[UIColor colorWithRed:.12 green:.25 blue:.4 alpha:1]:UIColor.clearColor;}
- (void)menuButton:(UIButton*)button {[self activateMenuRow:button.tag];}
- (void)activateMenuRow:(NSInteger)row {
    if(self.waitingAction){self.waitingAction=0;self.menuIndex=0;[self showRecompPage];return;}
    if(self.menuMode){
        const uint16_t actions[]={PAD_A,PAD_B,PAD_Y,PAD_X,PAD_Z,PAD_START};
        if(row<6){self.waitingAction=actions[row];self.menuIndex=0;}
        else if(row==6){ios_controller_reset_bindings();}
        else{self.menuMode=0;self.menuIndex=0;}
    }else if(row==0){
        if(UIScreen.mainScreen.maximumFramesPerSecond>=120)self.smoothEnabled=!self.smoothEnabled;
        smoothMotion=self.smoothEnabled;[NSUserDefaults.standardUserDefaults setBool:self.smoothEnabled forKey:@"recomp.smooth"];
        self.inputLink.preferredFrameRateRange=self.smoothEnabled?CAFrameRateRangeMake(120,120,120):CAFrameRateRangeMake(60,60,60);++fpsEpoch;
    }else if(row==1){self.fpsEnabled=!self.fpsEnabled;[NSUserDefaults.standardUserDefaults setBool:self.fpsEnabled forKey:@"recomp.fps"];++fpsEpoch;}
    else if(row==2){self.menuMode=1;self.menuIndex=0;}
    else{[self toggleRecomp];return;}
    [self showRecompPage];
}
- (void)menuInput:(uint32_t)pressed {
    if(self.waitingAction){for(unsigned i=0;i<11;++i)if(pressed&(1u<<i)){ios_controller_assign(i,self.waitingAction);self.waitingAction=0;self.menuIndex=0;[self showRecompPage];break;}return;}
    NSInteger n=self.menuButtons.count;if(!n)return;
    if(pressed&(1u<<7))self.menuIndex=(self.menuIndex+n-1)%n;
    if(pressed&(1u<<8))self.menuIndex=(self.menuIndex+1)%n;
    if(pressed&1u)[self activateMenuRow:self.menuIndex];
    else if(pressed&2u){if(self.menuMode){self.menuMode=0;self.menuIndex=0;[self showRecompPage];}else [self toggleRecomp];}
    [self highlightMenu];
}
