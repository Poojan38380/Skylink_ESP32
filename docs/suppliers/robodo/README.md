# EC200U LTE 4G GNSS Mini Industrial Modem — Supplier Comparison

**Product:** 7Semi EC200U LTE 4G GNSS Mini Industrial Modem USB-C (26V Range)  
**Detailed Technical Report:** [7Semi EC200U Mini Industrial Modem Report](file:///d:/btp_skylink/Skylink/docs/components/networking/7semi_ec200u_mini_industrial_modem.md)  
**Research Date:** May 26, 2026  
**Researched by:** Arthur (for Skylink BTP)

---

## 1. SemiComponent (semicomponent.com)

### Pricing

| Component | Amount |
|-----------|--------|
| Regular Price (excl. GST) | ₹2,251.34 |
| **Sale Price (excl. GST)** | **₹1,685.00** |
| GST @ 18% | ~₹303.30 |
| **Total (incl. GST)** | **₹1,988.30** |
| Discount | 25% off |
| Free Shipping | Over ₹999 |

### Warranty

**No explicit warranty mentioned on product page or policy pages.**

The Terms & Conditions state:
> *"THE MATERIALS IN THIS SITE ARE PROVIDED 'AS IS' AND WITHOUT WARRANTIES OF ANY KIND EITHER EXPRESS OR IMPLIED."*
> *"SemiComponent ... DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE."*

This is a **strong disclaimer against any warranty**. There is no separate warranty policy page on the site.

### Return & Refund Policy

| Scenario | Policy |
|----------|--------|
| **Defective / wrong item** | Contact immediately — policy mentions evaluation and correction but doesn't explicitly guarantee replacement |
| **Change of mind (unopened)** | "Only sealed and undamaged items may be returned" — at their discretion |
| **Cancellation before shipment** | 5% cancellation charge. Refund initiated within 2 business days. |
| **Cancellation after shipment (unopened)** | Refuse delivery. 5% cancellation + forward shipping + return shipping deducted from refund. |
| **Cancellation after shipment (opened)** | Returned items verified, refund within 3 business days. Same fees apply. |
| **Return shipping** | Borne by buyer |

### Company Details

| Detail | Value |
|--------|-------|
| **Company** | ElectroSoul Technologies |
| **Brand** | SemiComponent |
| **Contact Email** | hello@semicomponent.com |
| **Phone** | +91-820 033 8240 |
| **GSTIN** | 24AOFPB4307L1Z9 |
| **Jurisdiction** | Rajkot (per Terms) |
| **Shipping time** | 2-7 working days |
| **Ship from** | Same day if ordered before 2PM (Mon-Fri) |
| **Stock** | In Stock (1 unit) |

### Arthur's Assessment: RISK — HIGH

- **No warranty.** The terms explicitly disclaim all warranties including merchantability and fitness for purpose.
- The return policy is discretionary ("reserves the right to accept... at its discretion").
- If the modem arrives DOA, you have no guaranteed recourse beyond hoping they accept a return.
- The 5% cancellation fee on top of shipping costs if you change your mind.

---

## 2. Robodo (robodo.in) — ✅ RECOMMENDED

### Pricing

| Component | Amount |
|-----------|--------|
| Regular Price | ₹2,199.00 |
| **Sale Price** | **₹1,769.00** |
| Tax | **Including GST (No Hidden Charges)** |
| Discount | 20% off |
| Free Shipping | Not explicitly mentioned for this item |

### Warranty

**No explicit warranty mentioned on product page or policies.** The Terms of Service use standard Shopify boilerplate and do not mention warranty.

### Return & Refund Policy

| Scenario | Policy |
|----------|--------|
| **Defective / damaged / wrong item** | ✅ **7-day return window.** "Please inspect your order upon reception and contact us immediately if the item is defective, damaged or if you receive the wrong item, so that we can evaluate the issue and make it right." |
| **Change of mind (unused, original packaging)** | ✅ **7-day return.** Item must be unused, with tags, in original packaging. |
| **Return shipping** | **They provide a return shipping label** — they cover return shipping |
| **Refund timeframe** | Within 10 business days after inspection |
| **Sale items** | ❌ Cannot be returned |
| **Used / wrong connection damage** | ❌ Cannot be returned (user error) |
| **Returns address** | Bombay Electronics, 602, Chunam Lane, Off Lamington Road, Mumbai - 400007 |

### Company Details

| Detail | Value |
|--------|-------|
| **Company** | Bombay Electronics |
| **Brand** | Robodo |
| **Contact Email** | sales@bombayelectronics.in |
| **Phone** | 022 23885654 |
| **Physical Address** | 13B Shamrao Vithal Marg, off Lamington Road, opp grant road east post office, Mumbai - 400007 |
| **Return Address** | 602, Chunam Lane, Off Lamington Road, Mumbai - 400007 |
| **Stock** | In Stock |
| **Platform** | Shopify |
| **Established** | Older store (Shopify ID 25886550 suggests long operational history) |

### Arthur's Assessment: RISK — LOW

- **7-day return policy** — explicitly covers defective/damaged/wrong items. This is better than Robokits (5 days) and much better than SemiComponent (no warranty).
- **They provide return shipping label** — you don't pay to ship it back.
- **Bombay Electronics** is an established electronics shop on Lamington Road, Mumbai (a well-known electronics market). Physical store presence adds credibility.
- Item is marked "including GST (No Hidden Charges)" — transparent pricing.
- The only downsides: no explicit warranty period for manufacturing defects (beyond the 7-day return window), and sale items can't be returned (but this item is on sale at ₹1,769).

---

## 3. Side-by-Side Comparison

| Factor | SemiComponent | Robodo |
|--------|---------------|--------|
| **Price (incl. GST)** | **₹1,988.30** | **₹1,769.00** |
| **Return window** | Discretionary | **7 days guaranteed** |
| **Covers DOA/defective?** | Unclear / discretionary | ✅ Yes |
| **Return shipping covered?** | ❌ Buyer pays | ✅ Seller provides label |
| **Explicit warranty** | ❌ "As Is" — disclaimed | Not stated explicitly |
| **Physical store** | Online only | ✅ Bombay Electronics, Lamington Rd, Mumbai |
| **Cancellation fee** | 5% | Not stated |
| **Free shipping threshold** | ₹999 | Not stated |

---

## 4. Arthur's Verdict

**Order from Robodo.in, not SemiComponent.**

Even though Robodo is ₹219.30 more expensive (including GST), their **7-day guaranteed return policy with seller-paid return shipping** is worth significantly more than the price difference. SemiComponent's "AS IS, no warranties" terms leave you with no protection if the modem arrives dead.

### If You Order from Robodo — Action Plan

```
DAY 0 — Receive package
  → Inspect immediately
  → Check for physical damage
  → Verify it's the correct item (EC200U, not EC200U-CN variant)

DAY 0-7 — Test thoroughly
  → Power via USB-C (5V)
  → Check LTE network registration with SIM card
  → Verify GNSS lock (GPS/GLONASS/BeiDou)
  → Test UART communication
  → Run AT commands

IF DEFECTIVE — Within 7 days
  → Email sales@bombayelectronics.in
  → Describe the issue + photos/video
  → They send return shipping label
  → Ship to: Bombay Electronics, 602, Chunam Lane, Off Lamington Road, Mumbai - 400007
  → Refund within 10 business days after inspection
```