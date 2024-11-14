use anyhow::Context;
use leptos::{prelude::*, task::spawn_local};
use web_sys::{wasm_bindgen::JsCast, HtmlInputElement, SubmitEvent};

use crate::{api::{self, schedule::{Schedule, Time}}, web::{self, push_right}};

#[component]
pub fn home() -> impl IntoView {
	let schedule = LocalResource::new(|| async move {
		api::retry(api::schedule::schedule).await
	});

	view! {
		<Suspense fallback=web::loading>
			{move || Suspend::new(async move { view! {
				<LoadedHome schedule=schedule.await />
			}})}
		</Suspense>
	}
}

#[component]
fn loaded_home(schedule: Schedule) -> impl IntoView {
	fn parse_time(hhmm: String) -> anyhow::Result<Time> {
		let mut parts = hhmm.split(':');

		let hours = parts.next().context("invalid time")?
			.parse().context("invalid hours")?;

		let minutes = parts.next().context("invalid time")?
			.parse().context("invalid minutes")?;

		assert_eq!(parts.next(), None);

		Ok(Time::new(hours, minutes))
	}

	fn format_time(time: Time) -> String {
		format!("{:02}:{:02}", time.hours(), time.minutes())
	}

	let on_submit_schedule = |event: SubmitEvent| {
		let open_at = document().get_element_by_id("schedule-open-at")
			.expect("input element doesn't exist")
			.dyn_into::<HtmlInputElement>().expect("invalid element")
			.value();

		let close_at = document().get_element_by_id("schedule-close-at")
			.expect("input element doesn't exist")
			.dyn_into::<HtmlInputElement>().expect("invalid element")
			.value();

		let open_at = parse_time(open_at).expect("invalid opening time value");
		let close_at = parse_time(close_at).expect("invalid closing time value");

		let schedule = Schedule::new(open_at, close_at);

		log::info!("schedule set: {:?}", schedule);

		spawn_local(async move {
			api::retry(|| api::schedule::set_schedule(schedule)).await;
		});
		
		event.prevent_default();
	};

	let open_at = format_time(schedule.open_at());
	let close_at = format_time(schedule.close_at());
	
	view! {
		<h1>Schedule:</h1>
		<form id="schedule-form" on:submit={on_submit_schedule}></form>
		<dt-schedule>
			<dt-schedule-part>
				<h2>Open:</h2>
				<input id="schedule-open-at" type="time" value={open_at} form="schedule-form"></input>
			</dt-schedule-part>
			<dt-schedule-part>
				<h2>Close:</h2>
				<input id="schedule-close-at" type="time" value={close_at} form="schedule-form"></input>
			</dt-schedule-part>
		</dt-schedule>
	
		<dt-button-list>
			{push_right()}
			<button class="button-accent" type="submit" form="schedule-form">Save</button>
		</dt-button-list>
	}
}
