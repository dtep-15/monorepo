use api::{networks::NetworkConnection, state::DeviceState};
use pages::{configuration_completed::ConfigurationCompleted, connecting::Connecting, home::Home};
use leptos::prelude::*;

use pages::configuration::Configuration;
use template::Template;

mod pages;
mod api;
mod template;
mod web;

fn main() {
	cfg_if::cfg_if! {
		if #[cfg(debug_assertions)] {
			console_error_panic_hook::set_once();
			
			wasm_logger::init(wasm_logger::Config::new(log::Level::Trace));
		} else {
			wasm_logger::init(wasm_logger::Config::new(log::Level::Warn));
		}
	}

	let base_uri = location().origin()
		.expect("no window.location origin");

	api::init(base_uri.into());
	
    mount_to_body(move || view! { <App /> });
}

#[component]
fn app() -> impl IntoView {
	let device_state = LocalResource::new(|| async {
		api::retry(api::state::state).await
	});

	view! {
		<Suspense fallback=web::loading>
			{move || Suspend::new(async move { view! {
				<LoadedApp device_state=device_state.await />
			}})}
		</Suspense>
	}
}

#[component]
fn loaded_app(device_state: DeviceState) -> impl IntoView {
	let (state, set_state) = signal(if device_state.is_configured {
		AppState::Home
	} else {
		AppState::Confguration
	});

	provide_context(state);
	provide_context(set_state);

	let device_state = LocalResource::new(move || async move {
		let device_state = api::retry(api::state::state).await;

		set_state(
			if device_state.is_configured {
				AppState::Home
			} else {
				AppState::Confguration
			}
		);
	});

	view! {
		<Suspense fallback=web::loading>
			{move || Suspend::new(async move { device_state.await; view! {
				<Template>
					{move || match state() {
						AppState::Confguration => view! {
							<Configuration />
						}.into_any(),
						AppState::ConfgurationConnecting(connection_request) => view! {
							<Connecting connection_request={connection_request} />
						}.into_any(),
						AppState::ConfigurationComplete => view! {
							<ConfigurationCompleted />
						}.into_any(),
						AppState::Home => view! {
							<Home />
						}.into_any(),
					}}
				</Template>
			} })}
		</Suspense>
	}
}

#[derive(Debug, Clone)]
pub enum AppState {
	Confguration,
	ConfgurationConnecting(NetworkConnection),
	ConfigurationComplete,
	Home,
}

pub fn app_state() -> ReadSignal<AppState> {
	use_context()
		.expect("state machine context not available")
}

pub fn set_app_state() -> WriteSignal<AppState> {
	use_context()
		.expect("state machine context not available")
}
