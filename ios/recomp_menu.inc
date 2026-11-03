// UIKit methods included inside GameController. Input remains live for this menu
// while guest execution, GPU submission, rumble and audio are parked.
- (void)syncPause {
    bool pause=self.menuOpen || !self.appActive;
    if(pause){
        ios_rumble_suspend();
        ios_lifecycle::gate().pause();
        if(self.started)irq_request();
        ios_lifecycle::gate().wait_for_renderer();
        if(self.started)ios_audio_suspend();
        [self.touchControls resetInput];
        PadState neutral{};neutral.connected=ios_controller_snapshot().connected;pad_set_state(0,neutral);
    }else if(ios_lifecycle::gate().resume()){
        ++fpsEpoch;if(self.started)ios_audio_resume();
        ios_rumble_resume();
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
    self.menuOpen=!self.menuOpen;self.menuIndex=0;
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
    self.recompTitle.text=@"Recomp 메뉴";
    [self.recompStack addArrangedSubview:self.recompTitle];
    NSArray<NSString*>* titles=@[[NSString stringWithFormat:@"Smooth motion : %@",self.smoothEnabled?@"켜짐":@"꺼짐"],[NSString stringWithFormat:@"FPS 표시 : %@",self.fpsEnabled?@"켜짐":@"꺼짐"],@"게임으로 돌아가기"];
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
    if(row==0){
        if(UIScreen.mainScreen.maximumFramesPerSecond>=120)self.smoothEnabled=!self.smoothEnabled;
        smoothMotion=self.smoothEnabled;[NSUserDefaults.standardUserDefaults setBool:self.smoothEnabled forKey:@"recomp.smooth"];
        self.inputLink.preferredFrameRateRange=self.smoothEnabled?CAFrameRateRangeMake(120,120,120):CAFrameRateRangeMake(60,60,60);++fpsEpoch;
    }else if(row==1){self.fpsEnabled=!self.fpsEnabled;[NSUserDefaults.standardUserDefaults setBool:self.fpsEnabled forKey:@"recomp.fps"];++fpsEpoch;}
    else{[self toggleRecomp];return;}
    [self showRecompPage];
}
- (void)menuInput:(uint32_t)pressed {
    NSInteger n=self.menuButtons.count;if(!n)return;
    if(pressed&(1u<<7))self.menuIndex=(self.menuIndex+n-1)%n;
    if(pressed&(1u<<8))self.menuIndex=(self.menuIndex+1)%n;
    if(pressed&1u)[self activateMenuRow:self.menuIndex];
    else if(pressed&2u)[self toggleRecomp];
    [self highlightMenu];
}
