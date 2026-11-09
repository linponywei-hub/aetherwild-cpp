export class CombatFeedback {
 constructor(){this.attackAt=-Infinity;this.impactAt=-Infinity;this.amount=0;}
 swing(now){this.attackAt=now;}
 hit(now,amount){if(!Number.isFinite(amount)||amount<=0)return false;this.impactAt=now;this.amount=amount;return true;}
 sample(now){const hitAge=now-this.impactAt;return {hold:hitAge>=0&&hitAge<.045,recoil:Math.max(0,1-hitAge/.18),swing:Math.max(0,1-(now-this.attackAt)/.24)};}
}
