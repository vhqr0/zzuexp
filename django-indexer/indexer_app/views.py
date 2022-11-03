from django.shortcuts import get_object_or_404
from django.urls import reverse, reverse_lazy
from django.templatetags.static import static
from django.http import HttpResponse, HttpResponseRedirect
from django.views.generic import TemplateView, RedirectView, ListView, DetailView, FormView
from django.views.generic.edit import UpdateView
from django.views.generic.detail import SingleObjectMixin
from django.views.decorators.http import require_GET
from django.contrib.auth import logout
from django.contrib.auth.mixins import LoginRequiredMixin
from django.contrib.auth.models import User

from .models import VerifyRecord, UserProfile, Category, Page
from .forms import LoginForm, SigninForm, SigninVerifyForm, AvatarUploadForm, CategoryAddForm, PageAddForm


class GenericFormTemplateMixin:
    template_name = 'indexer/form.djhtml'
    legend = 'Form'
    enctype = None

    def get_context_data(self, **kwargs):
        context = super().get_context_data(**kwargs)
        context['legend'] = self.legend
        context['enctype'] = self.enctype
        return context


class IndexView(ListView):
    template_name = 'indexer/index.djhtml'
    context_object_name = 'pages'

    def get_queryset(self):
        return Page.objects.order_by('-views')[:5]


class AboutView(TemplateView):
    template_name = 'indexer/about.djhtml'


class LoginView(GenericFormTemplateMixin, FormView):
    form_class = LoginForm
    legend = 'Login'

    def form_valid(self, form):
        form.login(self.request)
        return super().form_valid(form)

    def get_success_url(self):
        next = self.request.GET.get('next')
        return next if next else reverse('indexer:index')


class SigninView(GenericFormTemplateMixin, FormView):
    form_class = SigninForm
    legend = 'Signin'

    def form_valid(self, form):
        self.pk = form.save()
        return super().form_valid(form)

    def get_success_url(self):
        return reverse('indexer:signin-verify', args=(self.pk, ))


class SigninVerifyView(GenericFormTemplateMixin, FormView):
    form_class = SigninVerifyForm
    legend = 'Signin Verify'
    success_url = reverse_lazy('indexer:login')

    def get(self, request, pk, *args, **kwargs):
        self.pk = pk
        return super().get(request, *args, **kwargs)

    def post(self, request, pk, *args, **kwargs):
        self.pk = pk
        return super().post(request, *args, **kwargs)

    def get_form(self, form_class=None):
        form = super().get_form(form_class)
        form.record_pk = self.pk
        return form

    def form_valid(self, form):
        form.signin()
        return super().form_valid(form)


class LogoutView(RedirectView):
    url = reverse_lazy('indexer:index')

    def get(self, request, *args, **kwargs):
        logout(request)
        return super().get(request, *args, **kwargs)


class UserListView(ListView):
    template_name = 'indexer/user_list.djhtml'
    paginate_by = 10

    def get_queryset(self):
        return User.objects.order_by('id')


class UserDetailView(DetailView):
    model = User
    template_name = 'indexer/user_detail.djhtml'

    def get_context_data(self, **kwargs):
        context = super().get_context_data(**kwargs)
        context['profile'] = UserProfile.get_profile(self.object)
        return context


class ProfileUpdateView(LoginRequiredMixin, GenericFormTemplateMixin,
                        UpdateView):
    model = UserProfile
    fields = ['website', 'self_introduction']

    def get_object(self, queryset=None):
        return UserProfile.get_profile(self.request.user)

    def get_success_url(self):
        return reverse('indexer:user-detail', args=(self.request.user.pk, ))


@require_GET
def avatar(request, pk):
    user = get_object_or_404(User, pk=pk)
    if hasattr(user, 'avatar'):
        return HttpResponse(user.avatar.data,
                            headers={'Content-Type': 'image/png'})
    else:
        return HttpResponseRedirect(
            static('indexer/images/default_avatar.png'))


class AvatarUploadView(GenericFormTemplateMixin, FormView):
    form_class = AvatarUploadForm
    legend = 'Avatar Upload'
    enctype = 'multipart/form-data'

    def form_valid(self, form):
        form.save(self.request)
        return super().form_valid(form)

    def get_success_url(self):
        return reverse('indexer:user-detail', args=(self.request.user.pk, ))


class CategoryListView(ListView):
    template_name = 'indexer/category_list.djhtml'
    paginate_by = 10

    def get_queryset(self):
        return Category.objects.order_by('-likes')


class CategoryDetailView(SingleObjectMixin, ListView):
    template_name = 'indexer/category_detail.djhtml'
    paginate_by = 10

    def get(self, request, *args, **kwargs):
        self.object = self.get_object(queryset=Category.objects.all())
        return super().get(request, *args, **kwargs)

    def get_context_data(self, **kwargs):
        context = super().get_context_data(**kwargs)
        context['category'] = self.object
        return context

    def get_queryset(self):
        return self.object.page_set.order_by('-views')


class CategoryAddView(LoginRequiredMixin, GenericFormTemplateMixin, FormView):
    form_class = CategoryAddForm
    legend = 'Add Category'
    success_url = reverse_lazy('indexer:index')
    login_url = reverse_lazy('indexer:login')

    def form_valid(self, form):
        form.save()
        return super().form_valid(form)


class PageAddView(LoginRequiredMixin, GenericFormTemplateMixin, FormView):
    form_class = PageAddForm
    legend = 'Add Page'
    success_url = reverse_lazy('indexer:index')
    login_url = reverse_lazy('indexer:login')

    def form_valid(self, form):
        form.save()
        return super().form_valid(form)
